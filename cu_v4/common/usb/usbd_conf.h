#ifndef __USBD_CONF__H__
#define __USBD_CONF__H__

#include "usb_conf.h"

#define USBD_CFG_MAX_NUM 1
#define USBD_ITF_MAX_NUM 1
#define USB_MAX_STR_DESC_SIZ 200

#define USBD_SELF_POWERED
#define XFERSIZE 1024

#define DFU_IN_EP 0x80
#define DFU_OUT_EP 0x00

#define TRANSFER_SIZE_BYTES(sze) ((uint8_t)(sze)),	   /* XFERSIZEB0 */ \
								 ((uint8_t)(sze >> 8)) /* XFERSIZEB1 */

#endif //__USBD_CONF__H__
