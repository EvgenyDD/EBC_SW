#ifndef USB_CONF_H__
#define USB_CONF_H__

#include "stm32f4xx.h"

#define USE_USB_OTG_FS
#define USB_OTG_FS_CORE

#ifdef USB_OTG_FS_CORE
#define RX_FIFO_FS_SIZE 128
#define TX0_FIFO_FS_SIZE 64
#define TX1_FIFO_FS_SIZE 64
#define TX2_FIFO_FS_SIZE 64
#define TX3_FIFO_FS_SIZE 64
#endif

#define USE_DEVICE_MODE

#define __ALIGN_BEGIN
#define __ALIGN_END
#define __packed __attribute__((__packed__))

#endif // USB_CONF_H__
