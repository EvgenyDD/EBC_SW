#ifndef __USB_DESC_H
#define __USB_DESC_H

#include "usbd_req.h"

#define USB_DEVICE_DESCRIPTOR_TYPE 0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE 0x02
#define USB_STRING_DESCRIPTOR_TYPE 0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE 0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE 0x05
#define USB_SIZ_DEVICE_DESC 18
#define USB_SIZ_STRING_LANGID 4

#define DFU_DESCRIPTOR_TYPE 0x21
#define USB_DFU_DESC_SIZ 9
#define USB_DFU_CONFIG_DESC_SIZ (18 + (9 * USBD_ITF_MAX_NUM))

/**********  Descriptor of DFU interface 0 Alternate setting n ****************/
#define USBD_DFU_IF_DESC(n) 0x09,																		  /* bLength: Interface Descriptor size */                \
							USB_INTERFACE_DESCRIPTOR_TYPE,												  /* bDescriptorType */                                   \
							0x00,																		  /* bInterfaceNumber: Number of Interface */             \
							(n),																		  /* bAlternateSetting: Alternate setting */              \
							0x00,																		  /* bNumEndpoints*/                                      \
							0xFE,																		  /* bInterfaceClass: Application Specific Class Code */  \
							0x01,																		  /* bInterfaceSubClass : Device Firmware Upgrade Code */ \
							0x02,																		  /* nInterfaceProtocol: DFU mode protocol */             \
							USBD_IDX_INTERFACE_STR + (n) + 1 /* iInterface: Index of string descriptor */ /* 18 */

extern uint8_t USBD_DeviceDesc[USB_SIZ_DEVICE_DESC];
extern uint8_t USBD_StrDesc[USB_MAX_STR_DESC_SIZ];
extern uint8_t USBD_OtherSpeedCfgDesc[USB_LEN_CFG_DESC];
extern uint8_t USBD_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC];
extern uint8_t USBD_LangIDDesc[USB_SIZ_STRING_LANGID];
extern USBD_DEVICE USR_desc;

uint8_t *USBD_USR_DeviceDescriptor(uint8_t speed, uint16_t *length);
uint8_t *USBD_USR_LangIDStrDescriptor(uint8_t speed, uint16_t *length);
uint8_t *USBD_USR_ManufacturerStrDescriptor(uint8_t speed, uint16_t *length);
uint8_t *USBD_USR_ProductStrDescriptor(uint8_t speed, uint16_t *length);
uint8_t *USBD_USR_SerialStrDescriptor(uint8_t speed, uint16_t *length);
uint8_t *USBD_USR_ConfigStrDescriptor(uint8_t speed, uint16_t *length);
uint8_t *USBD_USR_InterfaceStrDescriptor(uint8_t speed, uint16_t *length);

#endif /* __USBD_DESC_H */
