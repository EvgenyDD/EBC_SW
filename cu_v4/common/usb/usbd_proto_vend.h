#ifndef USB_PROTO_VEND_H__
#define USB_PROTO_VEND_H__

#include "usb_core.h"
#include <stdbool.h>

void usbd_proto_vendor_handler(void *pdev, USB_SETUP_REQ *req);
bool usbd_proto_is_idle(void);
void usbd_proto_vendor_tx_sent(void *pdev);
void usbd_proto_vendor_rx_received(void *pdev);

#endif // USB_PROTO_VEND_H__