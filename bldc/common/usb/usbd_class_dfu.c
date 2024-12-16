#include "usb_hw.h"
#include "usbd_core_dfu.h"
#include "usbd_desc.h"
#include "usbd_usr.h"

#ifdef USBD_CLASS_DFU
static uint8_t usbd_dfu_init(void *pdev, uint8_t cfgidx) { return USBD_OK; }
static uint8_t usbd_dfu_deinit(void *pdev, uint8_t cfgidx) { return USBD_OK; }

USBD_Class_cb_TypeDef usb_class_cb = {
	usbd_dfu_init,
	usbd_dfu_deinit,
	usbd_dfu_setup,
	NULL,
	usbd_dfu_ep0_rx_ready,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	usbd_get_cfg_desc,
};

void usb_poll(uint32_t diff_ms) { usbd_dfu_poll(diff_ms); }
#endif