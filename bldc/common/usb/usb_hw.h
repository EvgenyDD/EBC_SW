#ifndef USB_BSP_H__
#define USB_BSP_H__

#include "usbd_core.h"
#include <stdbool.h>
#include <stdint.h>

void usb_init(void);
void usb_disconnect(void);
void usb_poll(uint32_t diff_ms);
bool usb_is_configured(void);

extern USBD_Class_cb_TypeDef usb_class_cb;

#endif // USB_BSP_H__