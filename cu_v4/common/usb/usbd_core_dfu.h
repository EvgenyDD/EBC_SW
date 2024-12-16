#ifndef USBD_CORE_DFU_H__
#define USBD_CORE_DFU_H__

#include "usbd_ioreq.h"
#include <stdbool.h>

#ifdef USBD_DFU_USES_SUB_FLASHING
#include "dfu_sub_flash.h"
#endif

#define USB_DFU_CONFIG_DESC_SIZ (18 + (9 * USBD_ITF_MAX_NUM))

// ******* Descriptor of DFU interface 0 Alternate setting n
#define USBD_DFU_IF_DESC(n) USB_LEN_DFU_DESC,				  /* bLength */                                           \
							USB_DESC_TYPE_INTERFACE,		  /* bDescriptorType */                                   \
							0x00,							  /* bInterfaceNumber */                                  \
							(n),							  /* bAlternateSetting: alternate setting */              \
							0x00,							  /* bNumEndpoints*/                                      \
							USB_INTERFACE_CLASS_APP_SPECIFIC, /* bInterfaceClass: application Specific Class Code */  \
							0x01,							  /* bInterfaceSubClass : Device Firmware Upgrade Code */ \
							0x02,							  /* nInterfaceProtocol: DFU mode protocol */             \
							USBD_IDX_INTERFACE_STR + (n) + 1  /* iInterface: index of string descriptor */

enum
{
	DFU_DETACH = 0,
	DFU_DNLOAD = 1,
	DFU_UPLOAD,
	DFU_GETSTATUS,
	DFU_CLRSTATUS,
	DFU_GETSTATE,
	DFU_ABORT
};

uint8_t usbd_dfu_setup(void *pdev, USB_SETUP_REQ *req);
uint8_t usbd_dfu_ep0_rx_ready(void *pdev);
void usbd_dfu_poll(uint32_t diff_ms);

#endif // USBD_CORE_DFU_H__