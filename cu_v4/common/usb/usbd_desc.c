#include "usbd_desc.h"
#include "platform.h"
#include "usb_regs.h"
#include "usbd_conf.h"
#include "usbd_core.h"
#include "usbd_req.h"

#define USBD_VID 0x0483
#define USBD_PID 0xDF11

#define DEVICE_NAME "USB Device"

#define USBD_LANGID_STRING 0x409
#define USBD_MANUFACTURER_STRING "Open Source"
#define USBD_PRODUCT_HS_STRING DEVICE_NAME
#define USBD_PRODUCT_FS_STRING DEVICE_NAME
#define USBD_INTERFACE_FS_STRING DEVICE_NAME
#define USBD_INTERFACE_HS_STRING DEVICE_NAME
#define USBD_CONFIGURATION_FS_STRING DEVICE_NAME
#define USBD_CONFIGURATION_HS_STRING DEVICE_NAME

USBD_DEVICE USR_desc = {
	USBD_USR_DeviceDescriptor,
	USBD_USR_LangIDStrDescriptor,
	USBD_USR_ManufacturerStrDescriptor,
	USBD_USR_ProductStrDescriptor,
	USBD_USR_SerialStrDescriptor,
	USBD_USR_ConfigStrDescriptor,
	USBD_USR_InterfaceStrDescriptor,
};

__ALIGN_BEGIN uint8_t USBD_DeviceDesc[USB_SIZ_DEVICE_DESC] __ALIGN_END = {
	0x12,						/* bLength */
	USB_DEVICE_DESCRIPTOR_TYPE, /* bDescriptorType */
	0x00,						/* bcdUSB */
	0x02,
	0x00,				  /* bDeviceClass */
	0x00,				  /* bDeviceSubClass */
	0x00,				  /* bDeviceProtocol */
	USB_OTG_MAX_EP0_SIZE, /* bMaxPacketSize */
	LOBYTE(USBD_VID),	  /* idVendor */
	HIBYTE(USBD_VID),	  /* idVendor */
	LOBYTE(USBD_PID),	  /* idVendor */
	HIBYTE(USBD_PID),	  /* idVendor */
	0x00,				  /* bcdDevice rel. 2.00 */
	0x02,
	USBD_IDX_MFC_STR,	  /* Index of manufacturer string */
	USBD_IDX_PRODUCT_STR, /* Index of product string */
	USBD_IDX_SERIAL_STR,  /* Index of serial number string */
	USBD_CFG_MAX_NUM	  /* bNumConfigurations */
};						  /* USB_DeviceDescriptor */

__ALIGN_BEGIN uint8_t USBD_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END = {
	USB_LEN_DEV_QUALIFIER_DESC,
	USB_DESC_TYPE_DEVICE_QUALIFIER,
	0x00,
	0x02,
	0x00,
	0x00,
	0x00,
	0x40,
	0x01,
	0x00,
};

__ALIGN_BEGIN uint8_t USBD_LangIDDesc[USB_SIZ_STRING_LANGID] __ALIGN_END = {
	USB_SIZ_STRING_LANGID,
	USB_DESC_TYPE_STRING,
	LOBYTE(USBD_LANGID_STRING),
	HIBYTE(USBD_LANGID_STRING),
};

__ALIGN_BEGIN uint8_t usbd_dfu_CfgDesc[USB_DFU_CONFIG_DESC_SIZ] __ALIGN_END = {
	0x09,							   /* bLength: Configuration Descriptor size */
	USB_CONFIGURATION_DESCRIPTOR_TYPE, /* bDescriptorType: Configuration */
	USB_DFU_CONFIG_DESC_SIZ,
	/* wTotalLength: Bytes returned */
	0x00,
	0x01, /*bNumInterfaces: 1 interface*/
	0x01, /*bConfigurationValue: Configuration value*/
	0x02, /*iConfiguration: Index of string descriptor describing the configuration*/
	0xC0, /*bmAttributes: bus powered and Supports Remote Wakeup */
	0x32, /*MaxPower 100 mA: this current is used for detecting Vbus*/
	/* 09 */

	/**********  Descriptor of DFU interface 0 Alternate setting 0 **************/
	USBD_DFU_IF_DESC(0), /* This interface is mandatory for all devices */

	/******************** DFU Functional Descriptor********************/
	0x09,				 /*blength = 9 Bytes*/
	DFU_DESCRIPTOR_TYPE, /* DFU Functional Descriptor*/
	0x0B,				 /*bmAttribute
							   bitCanDnload             = 1      (bit 0)
							   bitCanUpload             = 1      (bit 1)
							   bitManifestationTolerant = 0      (bit 2)
							   bitWillDetach            = 1      (bit 3)
							   Reserved                          (bit4-6)
							   bitAcceleratedST         = 0      (bit 7)*/
	0xFF,				 /*DetachTimeOut= 255 ms*/
	0x00,
	/*WARNING: In DMA mode the multiple MPS packets feature is still not supported
	 ==> In this case, when using DMA XFERSIZE should be set to 64 in usbd_conf.h */
	TRANSFER_SIZE_BYTES(XFERSIZE), /* TransferSize = 1024 Byte*/
	0x1A,						   /* bcdDFUVersion*/
	0x01
	/***********************************************************/
	/* 9*/
};

uint8_t usbd_str_serial[128] = {0};

__ALIGN_BEGIN uint8_t USBD_StrDesc[USB_MAX_STR_DESC_SIZ] __ALIGN_END;

static int int_to_unicode(uint32_t value, uint8_t *pbuf, uint8_t len)
{
	for(uint8_t idx = 0; idx < len; idx++, value <<= 4, pbuf[2 * idx + 1] = 0)
		pbuf[2 * idx + 0] = (value >> 28) + ((value >> 28) < 0xA ? '0' : ('A' - 10));
	return 2 * len;
}

static int str_to_unicode(const char *s, uint8_t *pbuf)
{
	uint32_t idx = 0;
	for(; *s; idx++, s++, pbuf[2 * idx + 1] = 0)
		pbuf[2 * idx + 0] = *s;
	return 2 * idx;
}

uint8_t *USBD_DFU_GetCfgDesc(uint8_t speed, uint16_t *length)
{
	*length = sizeof(usbd_dfu_CfgDesc);
	return usbd_dfu_CfgDesc;
}

uint8_t *USBD_USR_DeviceDescriptor(uint8_t speed, uint16_t *length)
{
	*length = sizeof(USBD_DeviceDesc);
	return USBD_DeviceDesc;
}

uint8_t *USBD_USR_LangIDStrDescriptor(uint8_t speed, uint16_t *length)
{
	*length = sizeof(USBD_LangIDDesc);
	return USBD_LangIDDesc;
}

uint8_t *USBD_USR_ProductStrDescriptor(uint8_t speed, uint16_t *length)
{
	USBD_GetString(speed == USB_OTG_SPEED_HIGH ? USBD_PRODUCT_HS_STRING : USBD_PRODUCT_FS_STRING, USBD_StrDesc, length);
	return USBD_StrDesc;
}

uint8_t *USBD_USR_ManufacturerStrDescriptor(uint8_t speed, uint16_t *length)
{
	USBD_GetString(USBD_MANUFACTURER_STRING, USBD_StrDesc, length);
	return USBD_StrDesc;
}

uint8_t *USBD_USR_SerialStrDescriptor(uint8_t speed, uint16_t *length)
{
	int p = 2;
	p += str_to_unicode(DEV, &usbd_str_serial[p]);
	p += str_to_unicode("_", &usbd_str_serial[p]);
	p += int_to_unicode(g_uid[0], &usbd_str_serial[p], 8);
	p += int_to_unicode(g_uid[1], &usbd_str_serial[p], 8);
	p += int_to_unicode(g_uid[2], &usbd_str_serial[p], 8);
	usbd_str_serial[0] = p;
	usbd_str_serial[1] = USB_DESC_TYPE_STRING;
	*length = p;
	return usbd_str_serial;
}

uint8_t *USBD_USR_ConfigStrDescriptor(uint8_t speed, uint16_t *length)
{
	USBD_GetString(speed == USB_OTG_SPEED_HIGH ? USBD_CONFIGURATION_HS_STRING : USBD_CONFIGURATION_FS_STRING, USBD_StrDesc, length);
	return USBD_StrDesc;
}

uint8_t *USBD_USR_InterfaceStrDescriptor(uint8_t speed, uint16_t *length)
{
	USBD_GetString(speed == USB_OTG_SPEED_HIGH ? USBD_INTERFACE_HS_STRING : USBD_INTERFACE_FS_STRING, USBD_StrDesc, length);
	return USBD_StrDesc;
}
