#ifndef USBD_DESC_H__
#define USBD_DESC_H__

#include "usbd_req.h"

#define CDC_IN_EP 0x81	/* EP1 for data IN */
#define CDC_OUT_EP 0x01 /* EP1 for data OUT */
#define CDC_CMD_EP 0x82 /* EP2 for CDC commands (IN) */

#define CDC_DATA_MAX_PACKET_SIZE 64 /* Endpoint IN & OUT Packet size */
#define CDC_CMD_PACKET_SZE 8		/* Control Endpoint Packet size */

uint8_t *usbd_get_cfg_desc(uint8_t speed, uint16_t *length);
void usdb_desc_init(void);

extern uint8_t usb_device_desc[USB_LEN_DEV_DESC];
extern uint8_t USBD_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC];
extern USBD_DEVICE usbd_usr_cb;

#endif // USBD_DESC_H__
