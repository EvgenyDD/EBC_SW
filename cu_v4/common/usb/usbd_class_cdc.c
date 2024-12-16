#include "usb_hw.h"
#include "usbd_core_cdc.h"
#include "usbd_desc.h"
#include "usbd_usr.h"

#ifdef USBD_CLASS_CDC
USBD_Class_cb_TypeDef usb_class_cb = {
	usbd_cdc_init,
	usbd_cdc_deinit,
	usbd_cdc_setup,
	NULL,
	usbd_cdc_ep0_rx_ready,
	usbd_cdc_data_in,
	usbd_cdc_data_out,
	usbd_cdc_sof,
	NULL,
	NULL,
	usbd_get_cfg_desc,
};

void usb_poll(uint32_t diff_ms) {}
#endif