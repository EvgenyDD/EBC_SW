#ifndef __USB_CONF__H__
#define __USB_CONF__H__

#include "stm32f4xx.h"

#define USBD_SELF_POWERED

#define USE_USB_OTG_FS
#define USB_OTG_FS_CORE

#ifdef USB_OTG_FS_CORE
#define RX_FIFO_FS_SIZE 160
#define TX0_FIFO_FS_SIZE 160
#define TX1_FIFO_FS_SIZE 0
#define TX2_FIFO_FS_SIZE 0
#define TX3_FIFO_FS_SIZE 0
#endif

#define USE_DEVICE_MODE

#define __ALIGN_BEGIN
#define __ALIGN_END
#define __packed __attribute__((__packed__))

#endif /* __USB_CONF__H__ */
