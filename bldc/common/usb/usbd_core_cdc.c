#include "usbd_core_cdc.h"
#include "usbd_desc.h"
#include <stdbool.h>

#define CDC_DATA_IN_PACKET_SIZE CDC_DATA_MAX_PACKET_SIZE
#define CDC_DATA_OUT_PACKET_SIZE CDC_DATA_MAX_PACKET_SIZE

#define CDC_IN_FRAME_INTERVAL 5 /* Number of frames between IN transfers */
#define APP_RX_DATA_SIZE 2048	/* Total size of IN buffer: APP_RX_DATA_SIZE*8/MAX_BAUDARATE*1000 should be > CDC_IN_FRAME_INTERVAL */

enum
{
	USB_CDC_IDLE = 0,
	USB_CDC_BUSY,
	USB_CDC_ZLP
};

__ALIGN_BEGIN static uint32_t cdc_alt_set __ALIGN_END = 0;
__ALIGN_BEGIN static uint8_t cdc_cmd_buf[CDC_CMD_PACKET_SZE] __ALIGN_END;
__ALIGN_BEGIN static uint8_t cdc_rx_buf[CDC_DATA_MAX_PACKET_SIZE] __ALIGN_END; // CDC ->app
__ALIGN_BEGIN static uint8_t cdc_tx_buf[APP_RX_DATA_SIZE] __ALIGN_END;
static uint32_t cdc_tx_buf_len = 0;

static uint32_t tx_push_ptr = 0; // app -> CDC
static uint32_t tx_pop_ptr = 0;	 // app -> CDC

static uint8_t cdc_tx_state = USB_CDC_IDLE;

static uint32_t cdc_cmd = 0xFF;
static uint32_t cdc_len = 0;

static bool lock_cdc_tx = false;

static LINE_CODING linecoding = {
	115200, // baud rate
	0x00,	// stop bits: 1
	0x00,	// parity: none
	0x08	// N of bits: 8
};

static uint16_t vcp_ctrl(uint32_t cmd, uint8_t *data, uint32_t len)
{
	switch(cmd)
	{
	case SET_LINE_CODING:
		linecoding.bitrate = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
		linecoding.format = data[4];
		linecoding.paritytype = data[5];
		linecoding.datatype = data[6];
		break;

	case GET_LINE_CODING:
		data[0] = (uint8_t)(linecoding.bitrate);
		data[1] = (uint8_t)(linecoding.bitrate >> 8);
		data[2] = (uint8_t)(linecoding.bitrate >> 16);
		data[3] = (uint8_t)(linecoding.bitrate >> 24);
		data[4] = linecoding.format;
		data[5] = linecoding.paritytype;
		data[6] = linecoding.datatype;
		break;

	case SEND_ENCAPSULATED_COMMAND:
	case GET_ENCAPSULATED_RESPONSE:
	case SET_COMM_FEATURE:
	case GET_COMM_FEATURE:
	case CLEAR_COMM_FEATURE:
	case SET_CONTROL_LINE_STATE:
	case SEND_BREAK:
	default: break;
	}
	return USBD_OK;
}

uint8_t usbd_cdc_ep0_rx_ready(void *pdev)
{
	if(cdc_cmd != NO_CMD) vcp_ctrl(cdc_cmd, cdc_cmd_buf, cdc_len);
	cdc_cmd = NO_CMD;
	return USBD_OK;
}

uint8_t usbd_cdc_init(void *pdev, uint8_t cfgidx)
{
	DCD_EP_Open(pdev, CDC_IN_EP, CDC_DATA_IN_PACKET_SIZE, USB_OTG_EP_BULK);
	DCD_EP_Open(pdev, CDC_OUT_EP, CDC_DATA_OUT_PACKET_SIZE, USB_OTG_EP_BULK);
	DCD_EP_Open(pdev, CDC_CMD_EP, CDC_CMD_PACKET_SZE, USB_OTG_EP_INT);

#ifdef USBD_CLASS_COMPOSITE_DFU_CDC
	uint8_t *pbuf = (uint8_t *)usb_device_desc;
	pbuf[4] = USB_DEVICE_CLASS_CDC;
	pbuf[5] = USB_DEVICE_SUBCLASS_CDC;
#endif

	DCD_EP_PrepareRx(pdev, CDC_OUT_EP, cdc_rx_buf, CDC_DATA_OUT_PACKET_SIZE); /* Prepare Out endpoint to receive next packet */
	return USBD_OK;
}

uint8_t usbd_cdc_deinit(void *pdev, uint8_t cfgidx)
{
	DCD_EP_Close(pdev, CDC_IN_EP);
	DCD_EP_Close(pdev, CDC_OUT_EP);
	DCD_EP_Close(pdev, CDC_CMD_EP);

	usb_cdc_restore_desc_composite();
	return USBD_OK;
}

void usb_cdc_restore_desc_composite(void)
{
#ifdef USBD_CLASS_COMPOSITE_DFU_CDC
	uint8_t *pbuf = (uint8_t *)usb_device_desc;
	pbuf[4] = USB_DEVICE_CLASS_COMPOSITE;
	pbuf[5] = USB_DEVICE_SUBCLASS_COMPOSITE;
#endif
}

uint8_t usbd_cdc_setup(void *pdev, USB_SETUP_REQ *req)
{
	switch(req->bmRequest & USB_REQ_TYPE_MASK)
	{
	case USB_REQ_TYPE_CLASS: // CDC Class requests
		if(req->wLength)	 // check if the request is a data setup packet
		{
			if(req->bmRequest & 0x80) // request is Device-to-Host
			{
				vcp_ctrl(req->bRequest, cdc_cmd_buf, req->wLength); // get the data to be sent to Host from interface layer
				USBD_CtlSendData(pdev, cdc_cmd_buf, req->wLength);	// send the data to the host
			}
			else // Host-to-Device request
			{
				cdc_cmd = req->bRequest;
				cdc_len = req->wLength;
				USBD_CtlPrepareRx(pdev, cdc_cmd_buf, req->wLength); // prepare the reception of the buffer over EP0
			}
		}
		else // no Data request
		{
			vcp_ctrl(req->bRequest, NULL, 0);
		}
		return USBD_OK;

	case USB_REQ_TYPE_STANDARD:
		switch(req->bRequest)
		{
		case USB_REQ_GET_DESCRIPTOR:
			USBD_CtlError(pdev, req);
			return USBD_FAIL;

		case USB_REQ_GET_INTERFACE:
			USBD_CtlSendData(pdev, (uint8_t *)&cdc_alt_set, 1);
			break;

		case USB_REQ_SET_INTERFACE:
			if((uint8_t)(req->wValue) < USBD_ITF_MAX_NUM)
			{
				cdc_alt_set = (uint8_t)(req->wValue);
			}
			else
			{
				USBD_CtlError(pdev, req);
			}
			break;

		default: break;
		}
		break;

	default:
		USBD_CtlError(pdev, req);
		return USBD_FAIL;
	}
	return USBD_OK;
}

uint8_t usbd_cdc_data_in(void *pdev, uint8_t epnum)
{
	if(cdc_tx_state == USB_CDC_BUSY)
	{
		if(cdc_tx_buf_len == 0)
		{
			cdc_tx_state = USB_CDC_IDLE;
		}
		else
		{
			uint16_t tx_ptr;
			uint16_t tx_size;
			if(cdc_tx_buf_len > CDC_DATA_IN_PACKET_SIZE)
			{
				tx_ptr = tx_pop_ptr;
				tx_size = CDC_DATA_IN_PACKET_SIZE;

				tx_pop_ptr += CDC_DATA_IN_PACKET_SIZE;
				cdc_tx_buf_len -= CDC_DATA_IN_PACKET_SIZE;
			}
			else
			{
				tx_ptr = tx_pop_ptr;
				tx_size = cdc_tx_buf_len;

				tx_pop_ptr += cdc_tx_buf_len;
				cdc_tx_buf_len = 0;
				if(tx_size == CDC_DATA_IN_PACKET_SIZE) cdc_tx_state = USB_CDC_ZLP;
			}

			DCD_EP_Tx(pdev, CDC_IN_EP, &cdc_tx_buf[tx_ptr], tx_size); // prepare the available data buffer to be sent on IN endpoint
			return USBD_OK;
		}
	}

	if(cdc_tx_state == USB_CDC_ZLP) // avoid any asynchronous transfer during ZLP
	{
		DCD_EP_Tx(pdev, CDC_IN_EP, NULL, 0); // send ZLP to indicate the end of the current transfer
		cdc_tx_state = USB_CDC_IDLE;
	}

	return USBD_OK;
}

int usbd_cdc_push_data(const uint8_t *data, uint32_t size)
{
	int count = 0;
	for(uint32_t i = 0; i < size; i++)
	{
		cdc_tx_buf[tx_push_ptr++] = data[i];
		count++;
		if(tx_push_ptr == APP_RX_DATA_SIZE)
		{
			tx_push_ptr = 0;
			count = 0;
		}
	}
	return count;
}

uint8_t usbd_cdc_data_out(void *pdev, uint8_t epnum)
{
	uint16_t cnt = ((USB_OTG_CORE_HANDLE *)pdev)->dev.out_ep[epnum].xfer_count; // get the received data buffer and update the counter
	usbd_cdc_rx(cdc_rx_buf, cnt);
	DCD_EP_PrepareRx(pdev, CDC_OUT_EP, cdc_rx_buf, CDC_DATA_OUT_PACKET_SIZE); // prepare Out endpoint to receive next packet
	return USBD_OK;
}

static void handle_usb_async_xfer(void *pdev)
{
	if(lock_cdc_tx == false && cdc_tx_state == USB_CDC_IDLE)
	{
		if(tx_pop_ptr == APP_RX_DATA_SIZE) tx_pop_ptr = 0;

		if(tx_pop_ptr == tx_push_ptr)
		{
			cdc_tx_state = USB_CDC_IDLE;
			return;
		}

		cdc_tx_buf_len = tx_pop_ptr > tx_push_ptr ? APP_RX_DATA_SIZE - tx_pop_ptr : tx_push_ptr - tx_pop_ptr;

		uint16_t tx_ptr;
		uint16_t tx_size;
		if(cdc_tx_buf_len > CDC_DATA_IN_PACKET_SIZE)
		{
			tx_ptr = tx_pop_ptr;
			tx_size = CDC_DATA_IN_PACKET_SIZE;
			tx_pop_ptr += CDC_DATA_IN_PACKET_SIZE;
			cdc_tx_buf_len -= CDC_DATA_IN_PACKET_SIZE;
			cdc_tx_state = USB_CDC_BUSY;
		}
		else
		{
			tx_ptr = tx_pop_ptr;
			tx_size = cdc_tx_buf_len;
			tx_pop_ptr += cdc_tx_buf_len;
			cdc_tx_buf_len = 0;
			cdc_tx_state = tx_size == CDC_DATA_IN_PACKET_SIZE ? USB_CDC_ZLP : USB_CDC_BUSY;
		}
		DCD_EP_Tx(pdev, CDC_IN_EP, &cdc_tx_buf[tx_ptr], tx_size);
	}
}

uint8_t usbd_cdc_sof(void *pdev)
{
	static uint32_t frame_count = 0;

	if(++frame_count == CDC_IN_FRAME_INTERVAL)
	{
		frame_count = 0;
		handle_usb_async_xfer(pdev); // check the data to be sent through IN pipe
	}
	return USBD_OK;
}

void usbd_cdc_lock(void) { lock_cdc_tx = true; }
void usbd_cdc_unlock(void) { lock_cdc_tx = false; }