#include "usbd_core_dfu.h"
#include "fw_header.h"
#include "platform.h"
#include "ret_mem.h"
#include "usbd_req.h"
#include <string.h>
#ifdef DFU_READS_CFG_SECTION
#include "config_system.h"
#endif

extern bool g_stay_in_boot;

#define USBD_BUF_SZ 1024
#define DISC_TO_MS 100

#if FW_TYPE == FW_LDR
#define FW_TARGET FW_APP
#define ADDR_ORIGIN ((uint32_t) & __app_start)
#define ADDR_END ((uint32_t) & __app_end)
#elif FW_TYPE == FW_APP
#define FW_TARGET FW_LDR
#define ADDR_ORIGIN ((uint32_t) & __ldr_start)
#define ADDR_END ((uint32_t) & __ldr_end)
#endif

uint8_t usbd_buffer[USBD_BUF_SZ] = {0};
uint32_t usbd_buffer_rtx_sz = 0;

static uint32_t usbd_dfu_alt_set = 0;
static uint32_t cnt_till_reset = 0;
static uint8_t fw_sts[3], fw_type = FW_TYPE, fw_index_sel = 0;
static volatile bool dl_pending = false, rx_filename_pending = false;
static bool dwnload_was = false;

static struct
{
	bool pending;
	bool offset_received;
	uint32_t offset;
	uint32_t size;
} upload = {0};

#ifdef USBD_DFU_USES_SUB_FLASHING
dfu_app_cb_t *sub_flash_cb = {NULL};
#endif

__ALIGN_BEGIN uint8_t usbd_dfu_cfg_desc[USB_DFU_CONFIG_DESC_SIZ] __ALIGN_END = {
	0x09,						 // bLength: Configuration Descriptor size
	USB_DESC_TYPE_CONFIGURATION, // bDescriptorType
	USB_DFU_CONFIG_DESC_SIZ,	 // wTotalLength
	0x00,						 //
	0x01,						 // bNumInterfaces: 1 interface
	0x01,						 // bConfigurationValue: Configuration value
	0x02,						 // iConfiguration: Index of string descriptor describing the configuration
	0xC0,						 // bmAttributes: bus powered and Supports Remote Wakeup
	(100 / 2),					 // max power 100 mA: this current is used for detecting Vbus

	// ******* Descriptor of DFU interface 0 Alternate setting 0
	USBD_DFU_IF_DESC(0), // This interface is mandatory for all devices

	// ******* DFU Functional Descriptor
	0x09,						   // blength = 9 Bytes
	USB_DESC_TYPE_DFU,			   // DFU Functional Descriptor
	0x0B,						   /* bmAttribute
										 bitCanDnload             = 1      (bit 0)
										 bitCanUpload             = 1      (bit 1)
										 bitManifestationTolerant = 0      (bit 2)
										 bitWillDetach            = 1      (bit 3)
										 Reserved                          (bit4-6)
										 bitAcceleratedST         = 0      (bit 7) */
	255,						   // detach timeout= 255 ms
	0x00,						   //
	TRANSFER_SIZE_BYTES(XFERSIZE), // WARNING: In DMA mode the multiple MPS packets feature is still not supported ==> when using DMA XFERSIZE should be 64
	0x1A,						   // bcdDFUVersion
	0x01,
};

uint8_t usbd_dfu_setup(void *pdev, USB_SETUP_REQ *req)
{
	switch(req->bmRequest & USB_REQ_TYPE_MASK)
	{
	case USB_REQ_TYPE_CLASS:
#ifdef USBD_DFU_USES_SUB_FLASHING
		if(sub_flash_cb) sub_flash_cb->resp.pend_mask = 0;
#endif
		switch(req->bRequest)
		{
		case DFU_DETACH:
#ifdef USBD_DFU_USES_SUB_FLASHING
			if(req->wValue) // SUB FLASHING
			{
				if(sub_flash_cb) sub_flash_cb->p_reboot(sub_flash_cb->dev_idx);
			}
			else
#endif
			{
				cnt_till_reset = DISC_TO_MS;
			}
			break;

		case DFU_GETSTATE:
			g_stay_in_boot = true;
#ifdef USBD_DFU_USES_SUB_FLASHING
			if(sub_flash_cb)
			{
				sub_flash_cb->p_get_fw_type(pdev, req, &sub_flash_cb->resp, sub_flash_cb->dev_idx);
			}
			else
			{
#endif
				USBD_CtlSendData(pdev, &fw_type, sizeof(fw_type));
#ifdef USBD_DFU_USES_SUB_FLASHING
			}
#endif
			break;

		case DFU_CLRSTATUS:
			g_stay_in_boot = true;
#ifdef USBD_DFU_USES_SUB_FLASHING
			sub_flash_cb = NULL;
#endif
			if(req->wLength) // SUB FLASHING
			{
#ifdef USBD_DFU_USES_SUB_FLASHING
				rx_filename_pending = true;
				usbd_buffer_rtx_sz = req->wLength;
				USBD_CtlPrepareRx(pdev, usbd_buffer, req->wLength);
#else
				USBD_CtlError(pdev, req);
#endif
			}
			break;

		case DFU_GETSTATUS:
			g_stay_in_boot = true;

#ifdef USBD_DFU_USES_SUB_FLASHING
			if(sub_flash_cb)
			{
				if(sub_flash_cb->p_get_sts(pdev, req, &sub_flash_cb->resp, sub_flash_cb->dev_idx)) USBD_CtlError(pdev, req);
			}
			else
			{
#endif
				fw_header_check_all();
				fw_sts[0] = g_fw_info[0].locked;
				fw_sts[1] = g_fw_info[1].locked;
				fw_sts[2] = g_fw_info[2].locked;
				USBD_CtlSendData(pdev, fw_sts, sizeof(fw_sts));
#ifdef USBD_DFU_USES_SUB_FLASHING
			}
#endif
			break;

		case DFU_UPLOAD:
			if(req->bmRequest & 0x80) // upload data
			{
				if(upload.pending &&
				   upload.offset_received &&
#ifdef DFU_READS_CFG_SECTION
				   req->wValue < FW_COUNT + 1 &&
#else
				   req->wValue < FW_COUNT &&
#endif
				   upload.size < USBD_BUF_SZ)
				{
					upload.pending = upload.offset_received = false;
#ifdef USBD_DFU_USES_SUB_FLASHING
					if(sub_flash_cb)
					{
						if(sub_flash_cb->p_rd(pdev, req, &sub_flash_cb->resp, sub_flash_cb->dev_idx, req->wValue, upload.offset)) USBD_CtlError(pdev, req);
					}
					else
					{
#endif // USBD_DFU_USES_SUB_FLASHING
#ifdef DFU_READS_CFG_SECTION
						if(req->wValue == FW_APP + 1)
						{
							if(config_validate() == CONFIG_STS_OK)
							{
								uint32_t size_to_send = config_get_size() - upload.offset;
								if(size_to_send > upload.size) size_to_send = upload.size;
								USBD_CtlSendData(pdev, (uint8_t *)CFG_ORIGIN + upload.offset, size_to_send);
							}
							else
							{
								USBD_CtlSendData(pdev, (uint8_t *)CFG_ORIGIN, 0);
							}
						}
						else
#endif // DFU_READS_CFG_SECTION
						{
							if(g_fw_info[req->wValue].locked ||
							   upload.offset >= g_fw_info[req->wValue].size)
							{
								USBD_CtlSendData(pdev, (uint8_t *)g_fw_info[req->wValue].addr, 0);
							}
							else
							{
								uint32_t size_to_send = g_fw_info[req->wValue].size - upload.offset;
								if(size_to_send > upload.size) size_to_send = upload.size;
								USBD_CtlSendData(pdev, (uint8_t *)g_fw_info[req->wValue].addr + upload.offset, size_to_send);
							}
						}
#ifdef USBD_DFU_USES_SUB_FLASHING
					}
#endif
				}
				else
				{
					USBD_CtlError(pdev, req);
				}
			}
			else // receive fw index, offset and size to read
			{
				if(req->wValue >= FW_COUNT + 1 || req->wLength != 4 + 4)
				{
					USBD_CtlError(pdev, req);
				}
				else
				{
					upload.pending = true;
					upload.offset_received = false;
					USBD_CtlPrepareRx(pdev, usbd_buffer, req->wLength);
				}
			}
			break;

		case DFU_DNLOAD:
			if(req->wLength > sizeof(usbd_buffer) || req->wLength < 1 + 4)
			{
				USBD_CtlError(pdev, req);
				break;
			}
			fw_index_sel = req->wValue;
			dl_pending = true;
			usbd_buffer_rtx_sz = req->wLength;
			USBD_CtlPrepareRx(pdev, usbd_buffer, req->wLength);
			break;

		default:
			USBD_CtlError(pdev, req);
			break;
		}
		break;

	case USB_REQ_TYPE_STANDARD:
		switch(req->bRequest)
		{
		case USB_REQ_GET_DESCRIPTOR:
		{
			uint16_t len = 0;
			uint8_t *pbuf = NULL;
			if((req->wValue >> 8) == USB_DESC_TYPE_DFU)
			{
				pbuf = usbd_dfu_cfg_desc + 9 + (9 * USBD_ITF_MAX_NUM);
				len = MIN(USB_LEN_DFU_DESC, req->wLength);
			}
			USBD_CtlSendData(pdev, pbuf, len);
		}
		break;

		case USB_REQ_GET_INTERFACE: USBD_CtlSendData(pdev, (uint8_t *)&usbd_dfu_alt_set, 1); break;

		case USB_REQ_SET_INTERFACE:
			if((uint8_t)(req->wValue) < USBD_ITF_MAX_NUM)
			{
				usbd_dfu_alt_set = (uint8_t)(req->wValue);
			}
			else
			{
				USBD_CtlError(pdev, req);
			}
			break;

		default: break;
		}
	default: break;
	}
	return USBD_OK;
}

uint8_t usbd_dfu_ep0_rx_ready(void *pdev)
{
	if(dl_pending)
	{
		g_stay_in_boot = true;
		uint32_t addr_off, size_to_write = usbd_buffer_rtx_sz - 4;
		memcpy(&addr_off, &usbd_buffer[0], 4);
#ifdef USBD_DFU_USES_SUB_FLASHING
		if(sub_flash_cb)
		{
			int sts = sub_flash_cb->p_wr(pdev, NULL, &sub_flash_cb->resp, sub_flash_cb->dev_idx, fw_index_sel, addr_off, &usbd_buffer[4], size_to_write);
			dl_pending = false;
			return sts;
		}
		else
		{
#endif
			int sts;
#ifdef DFU_READS_CFG_SECTION
			if(fw_index_sel == FW_APP + 1)
			{
				if(addr_off > CFG_END - CFG_ORIGIN || CFG_END - CFG_ORIGIN - size_to_write < addr_off)
				{
					dl_pending = false;
					return USBD_FAIL;
				}
				if(addr_off == 0) platform_flash_erase_flag_reset_sect_cfg();
				sts = platform_flash_write(CFG_ORIGIN + addr_off, &usbd_buffer[4], size_to_write);
			}
			else
#endif
			{
				if(addr_off > ADDR_END - ADDR_ORIGIN || ADDR_END - ADDR_ORIGIN - size_to_write < addr_off)
				{
					dl_pending = false;
					return USBD_FAIL;
				}
				if(addr_off == 0) platform_flash_erase_flag_reset();
				sts = platform_flash_write(ADDR_ORIGIN + addr_off, &usbd_buffer[4], size_to_write);
			}
			dwnload_was = true;
			dl_pending = false;
			if(sts) return USBD_FAIL;
#ifdef USBD_DFU_USES_SUB_FLASHING
		}
#endif
	}
	if(upload.pending)
	{
		memcpy(&upload.offset, &usbd_buffer[0], 4);
		memcpy(&upload.size, &usbd_buffer[4], 4);
		upload.offset_received = true;
	}
#ifdef USBD_DFU_USES_SUB_FLASHING
	if(rx_filename_pending)
	{
		rx_filename_pending = false;
		for(uint8_t i = 0; i < dfu_app_cb_count; i++)
		{
			uint32_t entry_len = strlen(dfu_app_cb[i].name);
			if(usbd_buffer_rtx_sz == entry_len)
			{
				if(strncmp((const char *)usbd_buffer, dfu_app_cb[i].name, entry_len) == 0)
				{
					sub_flash_cb = &dfu_app_cb[i];
					return USBD_OK;
				}
			}
		}
		return USBD_FAIL;
	}
#endif

	return USBD_OK;
}

void usbd_dfu_poll(uint32_t diff_ms)
{
	if(cnt_till_reset)
	{
		if(cnt_till_reset <= diff_ms)
		{
			if(!dwnload_was) ret_mem_set_bl_stuck(true);
			platform_reset();
		}
		if(cnt_till_reset > diff_ms) cnt_till_reset -= diff_ms;
	}
}