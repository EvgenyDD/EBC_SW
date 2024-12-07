#include "usb_hw.h"
#include "usbd_core_cdc.h"
#include "usbd_core_dfu.h"
#include "usbd_desc.h"
#include "usbd_usr.h"

#ifdef USBD_CLASS_COMPOSITE_DFU_CDC

static uint8_t usbd_dfu_cdc_init(void *pdev, uint8_t cfgidx)
{
	usbd_cdc_init(pdev, cfgidx);
	return USBD_OK;
}

static uint8_t usbd_dfu_cdc_deinit(void *pdev, uint8_t cfgidx)
{
	usbd_cdc_deinit(pdev, cfgidx);
	return USBD_OK;
}

static uint8_t usbd_dfu_cdc_setup(void *pdev, USB_SETUP_REQ *req)
{
	switch(req->bmRequest & USB_REQ_RECIPIENT_MASK)
	{
	case USB_REQ_RECIPIENT_INTERFACE:
		if(req->wIndex == 0)
		{
			usbd_cdc_lock();
			return usbd_dfu_setup(pdev, req);
		}
		else
		{
			return usbd_cdc_setup(pdev, req);
		}

	case USB_REQ_RECIPIENT_ENDPOINT:
		// if(req->wIndex == HID_EP) return usbd_dfu_setup(pdev, req); else
		return usbd_cdc_setup(pdev, req);

	default: break;
	}
	return USBD_OK;
}

static uint8_t usbd_dfu_cdc_ep0_tx_sent(void *pdev)
{
	usbd_dfu_ep0_tx_sent(pdev);
	return USBD_OK;
}

static uint8_t usbd_dfu_cdc_ep0_rx_ready(void *pdev)
{
	usbd_cdc_ep0_rx_ready(pdev);
	return usbd_dfu_ep0_rx_ready(pdev);
}

static uint8_t usbd_dfu_cdc_data_in(void *pdev, uint8_t epnum) { return usbd_cdc_data_in(pdev, epnum); }
static uint8_t usbd_dfu_cdc_data_out(void *pdev, uint8_t epnum) { return usbd_cdc_data_out(pdev, epnum); }
static uint8_t usbd_dfu_cdc_sof(void *pdev) { return usbd_cdc_sof(pdev); }

USBD_Class_cb_TypeDef usb_class_cb = {
	usbd_dfu_cdc_init,
	usbd_dfu_cdc_deinit,
	usbd_dfu_cdc_setup,
	usbd_dfu_cdc_ep0_tx_sent,
	usbd_dfu_cdc_ep0_rx_ready,
	usbd_dfu_cdc_data_in,
	usbd_dfu_cdc_data_out,
	usbd_dfu_cdc_sof,
	NULL,
	NULL,
	usbd_get_cfg_desc,
};

void usb_poll(uint32_t diff_ms) { usbd_dfu_poll(diff_ms); }

void USBD_USR_DeviceSuspended(void) { usb_cdc_restore_desc_composite(); }
#endif