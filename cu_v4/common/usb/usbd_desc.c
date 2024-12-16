#include "usbd_desc.h"
#include "platform.h"
#include "usb_regs.h"
#include "usbd_conf.h"
#include "usbd_core.h"
#include "usbd_core_dfu.h"
#include "usbd_def.h"
#include "usbd_req.h"

#if defined(USBD_CLASS_COMPOSITE_DFU_CDC)
// #define USE_STD_LIB_GEN
#include "md5.h"
#endif

#if defined(USBD_CLASS_COMPOSITE_DFU_CDC)
#define USBD_VID 0xF055
#define USBD_PID 0x1337
#elif defined(USBD_CLASS_DFU)
#define USBD_VID 0x0483
#define USBD_PID 0xDF11
#elif defined(USBD_CLASS_CDC)
#define USBD_VID 0x0483
#define USBD_PID 0x5740
#endif

#define DEVICE_NAME "USB Device"

#define USBD_LANGID_STRING 0x409
#define USBD_MANUFACTURER_STRING "Open Source"
#define USBD_PRODUCT_HS_STRING DEVICE_NAME
#define USBD_PRODUCT_FS_STRING DEVICE_NAME
#define USBD_INTERFACE_FS_STRING DEVICE_NAME
#define USBD_INTERFACE_HS_STRING DEVICE_NAME
#define USBD_CONFIGURATION_FS_STRING DEVICE_NAME
#define USBD_CONFIGURATION_HS_STRING DEVICE_NAME

#define USB_CDC_CONFIG_DESC_SIZ (67)
#define USB_CDC_DFU_CONFIG_DESC_SIZ (9 + 9 * USBD_ITF_MAX_NUM + USB_CDC_CONFIG_DESC_SIZ + 8)
#define USB_DFU_CONFIG_DESC_SIZ (18 + (9 * USBD_ITF_MAX_NUM))

static __ALIGN_BEGIN uint8_t usb_device_desc[USB_LEN_DEV_DESC] __ALIGN_END = {
	USB_LEN_DEV_DESC,	  // bLength
	USB_DESC_TYPE_DEVICE, // bDescriptorType
	0,					  // bcdUSB
	0x02,				  //
#if defined(USBD_CLASS_COMPOSITE_DFU_CDC)
	USB_DEVICE_CLASS_COMPOSITE,	   // bDeviceClass: Miscellaneous Device Class
	USB_DEVICE_SUBCLASS_COMPOSITE, // bDeviceSubClass: Common Class
	0x01,						   // bDeviceProtocol: Interface Association Descriptor
#elif defined(USBD_CLASS_DFU)
	0, /* bDeviceClass */
	0, /* bDeviceSubClass */
	0, /* bDeviceProtocol */
#elif defined(USBD_CLASS_CDC)
	USB_DEVICE_CLASS_CDC,	 // bDeviceClass
	USB_DEVICE_SUBCLASS_CDC, // bDeviceSubClass
	0,						 // bDeviceProtocol
#endif
	USB_OTG_MAX_EP0_SIZE, // bMaxPacketSize
	LOBYTE(USBD_VID),	  // idVendor
	HIBYTE(USBD_VID),	  // idVendor
	LOBYTE(USBD_PID),	  // idVendor
	HIBYTE(USBD_PID),	  // idVendor
	0,					  // bcdDevice rel. 2.00
	0x02,				  //
	USBD_IDX_MFC_STR,	  // Index of manufacturer string
	USBD_IDX_PRODUCT_STR, // Index of product string
	USBD_IDX_SERIAL_STR,  // Index of serial number string
	USBD_CFG_MAX_NUM	  // bNumConfigurations
};

__ALIGN_BEGIN uint8_t usbd_qualifier_desc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END = {
	USB_LEN_DEV_QUALIFIER_DESC,
	USB_DESC_TYPE_DEVICE_QUALIFIER,
	0,
	0x02,
	0,
	0,
	0,
	0x40,
	0x01,
	0,
};

static __ALIGN_BEGIN uint8_t usbd_lang_id_desc[USB_LEN_LANGID_DESC] __ALIGN_END = {
	USB_LEN_LANGID_DESC,
	USB_DESC_TYPE_STRING,
	LOBYTE(USBD_LANGID_STRING),
	HIBYTE(USBD_LANGID_STRING),
};

#if defined(USBD_CLASS_COMPOSITE_DFU_CDC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpacked"
struct __packed
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wData[7];
	uint8_t bVendorCode;
	uint8_t bPadding;
} usbd_os_str_desc = {
	sizeof(usbd_os_str_desc),
	USB_DESC_TYPE_STRING,
	u"MSFT100",
	USB_REQ_GET_OS_FEATURE_DESCRIPTOR,
	0,
};

struct __packed
{
	// Header
	uint32_t dwLength;
	uint16_t bcdVersion;
	uint16_t wIndex;
	uint8_t bCount;
	uint8_t bReserved1[7];
	// Function Section 1
	uint8_t bFirstInterfaceNumber;
	uint8_t bReserved2;
	uint8_t bCompatibleID[8];
	uint8_t bSubCompatibleID[8];
	uint8_t bReserved3[6];
} usbd_compat_id_desc = {
	sizeof(usbd_compat_id_desc),
	WINUSB_BCD_VERSION,
	WINUSB_REQ_GET_COMPATIBLE_ID_FEATURE_DESCRIPTOR, // wIndex
	1,												 // bCount
	{0},											 // bReserved1
	0,												 // bFirstInterfaceNumber
	1,												 // bReserved2
	"WINUSB",										 // bCompatibleID
	{0},											 // bSubCompatibleID
	{0},											 // bReserved3
};

struct winusb_ext_prop_desc_hdr
{
	// header
	uint32_t dwLength;
	uint16_t bcdVersion;
	uint16_t wIndex;
	uint16_t wNumFeatures;
} __packed;

struct winusb_ext_prop_feat_desc
{
	// feature no. 1
	uint32_t dwSize;
	uint32_t dwPropertyDataType;
	uint16_t wPropertyNameLength;
	const uint16_t bPropertyName[21];
	uint32_t dwPropertyDataLength;
	uint16_t bPropertyData[40];
} __packed;

struct winusb_ext_prop_desc
{
	struct winusb_ext_prop_desc_hdr header;
	struct winusb_ext_prop_feat_desc features[1];
} __packed;
#pragma GCC diagnostic pop

struct winusb_ext_prop_desc usbd_winusb_ex_prop_desc = {
	.header = {
		.dwLength = sizeof(struct winusb_ext_prop_desc_hdr) + sizeof(struct winusb_ext_prop_feat_desc),
		.bcdVersion = WINUSB_BCD_VERSION,
		.wIndex = WINUSB_REQ_GET_EXTENDED_PROPERTIES_OS_FEATURE_DESCRIPTOR,
		.wNumFeatures = 1,
	},
	.features = {{
		.dwSize = sizeof(struct winusb_ext_prop_feat_desc),
		.dwPropertyDataType = WINUSB_PROP_DATA_TYPE_REG_REG_MULTI_SZ,
		.wPropertyNameLength = sizeof(usbd_winusb_ex_prop_desc.features[0].bPropertyName),
		.bPropertyName = u"DeviceInterfaceGUIDs",
		.dwPropertyDataLength = sizeof(usbd_winusb_ex_prop_desc.features[0].bPropertyData),
		.bPropertyData = u"{00000000-0000-0000-0000-000000000000}\0",
	}},
};

__ALIGN_BEGIN static uint8_t usbd_cfg_desc[] __ALIGN_END = {
	0x09,						 // bLength
	USB_DESC_TYPE_CONFIGURATION, // bDescriptorType
	USB_CDC_DFU_CONFIG_DESC_SIZ, // wTotalLength
	0,							 //
	0x03,						 // bNumInterfaces: 3 interface (2 for CDC, 1 for DFU)
	0x01,						 // bConfigurationValue: Configuration value
	0x02,						 // iConfiguration: Index of string descriptor describing the configuration
	0xC0,						 // bmAttributes: bus powered and Supports Remote Wakeup
	(100 / 2),					 // max power 100 mA: this current is used for detecting Vbus

	// ******* Descriptor of DFU interface 0 Alternate setting 0
	USBD_DFU_IF_DESC(0), // This interface is mandatory for all devices

	// ******* DFU Functional Descriptor
	0x09,						   // blength
	USB_DESC_TYPE_DFU,			   // functional descriptor
	0x0B,						   /* bmAttribute
										 bitCanDnload             = 1      (bit 0)
										 bitCanUpload             = 1      (bit 1)
										 bitManifestationTolerant = 0      (bit 2)
										 bitWillDetach            = 1      (bit 3)
										 Reserved                          (bit4-6)
										 bitAcceleratedST         = 0      (bit 7) */
	255,						   // detach timeout= 255 ms
	0,							   //
	TRANSFER_SIZE_BYTES(XFERSIZE), // WARNING: In DMA mode the multiple MPS packets feature is still not supported ==> when using DMA XFERSIZE should be 64
	0x1A,						   // bcdDFUVersion
	0x01,

	// ******* IAD to associate the two CDC interfaces
	0x08, // bLength
	0x0B, // bDescriptorType
	0x01, // bFirstInterface
	0x02, // bInterfaceCount
	0x02, // bFunctionClass
	0x02, // bFunctionSubClass
	0x01, // bFunctionProtocol
	0,	  // iFunction

	// ******* CDC Interface Descriptor
	0x09,							// bLength: Interface Descriptor size
	USB_DESC_TYPE_INTERFACE,		// bDescriptorType: Interface
	0x01,							// bInterfaceNumber: Number of Interface
	0,								// bAlternateSetting: Alternate setting
	0x01,							// bNumEndpoints: One endpoints used
	USB_INTERFACE_CLASS_COMM_IFACE, // bInterfaceClass: Communication Interface Class
	0x02,							// bInterfaceSubClass: Abstract Control Model
	0x01,							// bInterfaceProtocol: Common AT commands
	0x01,							// iInterface

	// Header Functional Descriptor
	0x05, // bLength: Endpoint Descriptor size
	0x24, // bDescriptorType: CS_INTERFACE
	0,	  // bDescriptorSubtype: Header Func Desc
	0x10, // bcdCDC: spec release number
	0x01,

	// Call Management Functional Descriptor
	0x05, // bFunctionLength
	0x24, // bDescriptorType: CS_INTERFACE
	0x01, // bDescriptorSubtype: Call Management Func Desc
	0,	  // bmCapabilities: D0+D1
	0x02, // bDataInterface: 2

	// ACM Functional Descriptor
	0x04, // bFunctionLength
	0x24, // bDescriptorType: CS_INTERFACE
	0x02, // bDescriptorSubtype: Abstract Control Management desc
	0x02, // bmCapabilities

	// Union Functional Descriptor
	0x05, // bFunctionLength
	0x24, // bDescriptorType: CS_INTERFACE
	0x06, // bDescriptorSubtype: Union func desc
	0x01, // bMasterInterface: Communication class interface
	0x02, // bSlaveInterface0: Data Class Interface

	// ******* Endpoint 2 (CMD) Descriptor
	0x07,						 // bLength: Endpoint Descriptor size
	USB_DESC_TYPE_ENDPOINT,		 // bDescriptorType: Endpoint
	CDC_CMD_EP,					 // bEndpointAddress
	0x03,						 // bmAttributes: Interrupt
	LOBYTE(CDC_CMD_PACKET_SIZE), // wMaxPacketSize
	HIBYTE(CDC_CMD_PACKET_SIZE), //
	0xFF,						 // bInterval

	// ******* Data class interface descriptor
	0x09,					 // bLength: Endpoint Descriptor size
	USB_DESC_TYPE_INTERFACE, // bDescriptorType
	0x02,					 // bInterfaceNumber: Number of Interface
	0,						 // bAlternateSetting: Alternate setting
	0x02,					 // bNumEndpoints: Two endpoints used
	USB_INTERFACE_CLASS_CDC, // bInterfaceClass: CDC
	0,						 // bInterfaceSubClass
	0,						 // bInterfaceProtocol
	0,						 // iInterface

	// ******* Endpoint OUT Descriptor
	0x07,							  // bLength: Endpoint Descriptor size
	USB_DESC_TYPE_ENDPOINT,			  // bDescriptorType: Endpoint
	CDC_OUT_EP,						  // bEndpointAddress
	0x02,							  // bmAttributes: Bulk
	LOBYTE(CDC_DATA_MAX_PACKET_SIZE), // wMaxPacketSize
	HIBYTE(CDC_DATA_MAX_PACKET_SIZE), //
	0,								  // bInterval: ignore for Bulk transfer

	// ******* Endpoint IN Descriptor
	0x07,							  // bLength: Endpoint Descriptor size
	USB_DESC_TYPE_ENDPOINT,			  // bDescriptorType: Endpoint
	CDC_IN_EP,						  // bEndpointAddress
	0x02,							  // bmAttributes: Bulk
	LOBYTE(CDC_DATA_MAX_PACKET_SIZE), // wMaxPacketSize
	HIBYTE(CDC_DATA_MAX_PACKET_SIZE), //
	0,								  // bInterval
};
enum
{
	_check_usbd_cfg_desc = 1 / (sizeof(usbd_cfg_desc) == USB_CDC_DFU_CONFIG_DESC_SIZ ? 1 : 0),
	_check_usbd_os_str_desc = 1 / (sizeof(usbd_os_str_desc) == 18 ? 1 : 0),
	_check_usbd_compat_id_desc = 1 / (sizeof(usbd_compat_id_desc) == 40 ? 1 : 0),
	_check_usbd_desc_winusb_ex_prop = 1 / (sizeof(usbd_winusb_ex_prop_desc) == 146 ? 1 : 0),
};
#elif defined(USBD_CLASS_DFU)
__ALIGN_BEGIN static uint8_t usbd_cfg_desc[] __ALIGN_END = {
	0x09,						 // bLength
	USB_DESC_TYPE_CONFIGURATION, // bDescriptorType
	USB_DFU_CONFIG_DESC_SIZ,	 // wTotalLength
	0,							 //
	0x01,						 // bNumInterfaces: 1 interface
	0x01,						 // bConfigurationValue: Configuration value
	0x02,						 // iConfiguration: Index of string descriptor describing the configuration
	0xC0,						 // bmAttributes: bus powered and Supports Remote Wakeup
	(100 / 2),					 // max power 100 mA: this current is used for detecting Vbus

	// ******* Descriptor of DFU interface 0 Alternate setting 0
	USBD_DFU_IF_DESC(0), // This interface is mandatory for all devices

	// ******* DFU Functional Descriptor
	0x09,						   // blength
	USB_DESC_TYPE_DFU,			   // functional descriptor
	0x0B,						   /* bmAttribute
										 bitCanDnload             = 1      (bit 0)
										 bitCanUpload             = 1      (bit 1)
										 bitManifestationTolerant = 0      (bit 2)
										 bitWillDetach            = 1      (bit 3)
										 Reserved                          (bit4-6)
										 bitAcceleratedST         = 0      (bit 7) */
	255,						   // detach timeout= 255 ms
	0,							   //
	TRANSFER_SIZE_BYTES(XFERSIZE), // WARNING: In DMA mode the multiple MPS packets feature is still not supported ==> when using DMA XFERSIZE should be 64
	0x1A,						   // bcdDFUVersion
	0x01,
};
enum
{
	_check_usbd_cfg_desc = 1 / (sizeof(usbd_cfg_desc) == USB_DFU_CONFIG_DESC_SIZ ? 1 : 0),
};
#elif defined(USBD_CLASS_CDC)
__ALIGN_BEGIN static uint8_t usbd_cfg_desc[] __ALIGN_END = {
	0x09,						 // bLength
	USB_DESC_TYPE_CONFIGURATION, // bDescriptorType
	USB_CDC_CONFIG_DESC_SIZ,	 // wTotalLength
	0,							 //
	0x02,						 // bNumInterfaces: 2 interface
	0x01,						 // bConfigurationValue: Configuration value
	0,							 // iConfiguration: Index of string descriptor describing the configuration
	0xC0,						 // bmAttributes: bus powered
	(100 / 2),					 // max power 100 mA: this current is used for detecting Vbus

	// ******* CDC Interface Descriptor
	0x09,							// bLength: Interface Descriptor size
	USB_DESC_TYPE_INTERFACE,		// bDescriptorType: Interface
	0,								// bInterfaceNumber
	0,								// bAlternateSetting: Alternate setting
	0x01,							// bNumEndpoints: one endpoint used
	USB_INTERFACE_CLASS_COMM_IFACE, // bInterfaceClass: Communication Interface Class
	0x02,							// bInterfaceSubClass: Abstract Control Model
	0x01,							// bInterfaceProtocol: Common AT commands
	0,								// iInterface

	// Header Functional Descriptor
	0x05, // bLength: Endpoint Descriptor size
	0x24, // bDescriptorType: CS_INTERFACE
	0,	  // bDescriptorSubtype: Header Func Desc
	0x10, // bcdCDC: spec release number
	0x01,

	// Call Management Functional Descriptor
	0x05, // bFunctionLength
	0x24, // bDescriptorType: CS_INTERFACE
	0x01, // bDescriptorSubtype: Call Management Func Desc
	0,	  // bmCapabilities: D0+D1
	0x01, // bDataInterface: 1

	// ACM Functional Descriptor
	0x04, // bFunctionLength
	0x24, // bDescriptorType: CS_INTERFACE
	0x02, // bDescriptorSubtype: Abstract Control Management desc
	0x02, // bmCapabilities

	// Union Functional Descriptor
	0x05, // bFunctionLength
	0x24, // bDescriptorType: CS_INTERFACE
	0x06, // bDescriptorSubtype: Union func desc
	0,	  // bMasterInterface: Communication class interface
	0x01, // bSlaveInterface0: Data Class Interface

	// ******* Endpoint 2 (CMD) Descriptor
	0x07,						 // bLength: Endpoint Descriptor size
	USB_DESC_TYPE_ENDPOINT,		 // bDescriptorType: Endpoint
	CDC_CMD_EP,					 // bEndpointAddress
	0x03,						 // bmAttributes: Interrupt
	LOBYTE(CDC_CMD_PACKET_SIZE), // wMaxPacketSize
	HIBYTE(CDC_CMD_PACKET_SIZE), //
	0xFF,						 // bInterval

	// ******* Data class interface descriptor
	0x09,					 // bLength: Endpoint Descriptor size
	USB_DESC_TYPE_INTERFACE, // bDescriptorType
	0x01,					 // bInterfaceNumber: Number of Interface
	0,						 // bAlternateSetting: Alternate setting
	0x02,					 // bNumEndpoints: Two endpoints used
	USB_INTERFACE_CLASS_CDC, // bInterfaceClass: CDC
	0,						 // bInterfaceSubClass
	0,						 // bInterfaceProtocol
	0,						 // iInterface

	// ******* Endpoint OUT Descriptor
	0x07,							  // bLength: Endpoint Descriptor size
	USB_DESC_TYPE_ENDPOINT,			  // bDescriptorType: Endpoint
	CDC_OUT_EP,						  // bEndpointAddress
	0x02,							  // bmAttributes: Bulk
	LOBYTE(CDC_DATA_MAX_PACKET_SIZE), // wMaxPacketSize
	HIBYTE(CDC_DATA_MAX_PACKET_SIZE), //
	0,								  // bInterval: ignore for Bulk transfer

	// ******* Endpoint IN Descriptor
	0x07,							  // bLength: Endpoint Descriptor size
	USB_DESC_TYPE_ENDPOINT,			  // bDescriptorType: Endpoint
	CDC_IN_EP,						  // bEndpointAddress
	0x02,							  // bmAttributes: Bulk
	LOBYTE(CDC_DATA_MAX_PACKET_SIZE), // wMaxPacketSize
	HIBYTE(CDC_DATA_MAX_PACKET_SIZE), //
	0,								  // bInterval
};
enum
{
	_check_usbd_cfg_desc = 1 / (sizeof(usbd_cfg_desc) == USB_CDC_CONFIG_DESC_SIZ ? 1 : 0),
};
#endif

static uint8_t usbd_str_serial[128] = {0};
static __ALIGN_BEGIN uint8_t usbd_str_desc_buf[USB_MAX_STR_DESC_SIZ] __ALIGN_END;

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

static uint8_t *usbd_usr_device_desc(uint8_t speed, uint16_t *length)
{
	*length = sizeof(usb_device_desc);
	return usb_device_desc;
}

static uint8_t *usbd_usr_lang_id_desc(uint8_t speed, uint16_t *length)
{
	*length = sizeof(usbd_lang_id_desc);
	return usbd_lang_id_desc;
}

static uint8_t *usbd_usr_prod_desc(uint8_t speed, uint16_t *length)
{
	USBD_GetString((const uint8_t *)(speed == USB_OTG_SPEED_HIGH ? USBD_PRODUCT_HS_STRING : USBD_PRODUCT_FS_STRING), usbd_str_desc_buf, length);
	return usbd_str_desc_buf;
}

static uint8_t *usbd_usr_manuf_desc(uint8_t speed, uint16_t *length)
{
	USBD_GetString((const uint8_t *)USBD_MANUFACTURER_STRING, usbd_str_desc_buf, length);
	return usbd_str_desc_buf;
}

static uint8_t *usbd_usr_serial_desc(uint8_t speed, uint16_t *length)
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

static uint8_t *usbd_usr_cfg_desc(uint8_t speed, uint16_t *length)
{
	USBD_GetString((const uint8_t *)(speed == USB_OTG_SPEED_HIGH ? USBD_CONFIGURATION_HS_STRING : USBD_CONFIGURATION_FS_STRING), usbd_str_desc_buf, length);
	return usbd_str_desc_buf;
}

static uint8_t *usbd_usr_if_desc(uint8_t speed, uint16_t *length)
{
	USBD_GetString((const uint8_t *)(speed == USB_OTG_SPEED_HIGH ? USBD_INTERFACE_HS_STRING : USBD_INTERFACE_FS_STRING), usbd_str_desc_buf, length);
	return usbd_str_desc_buf;
}

static uint8_t *usbd_usr_os_str_desc(uint8_t speed, uint16_t *length)
{
#if defined(USBD_CLASS_COMPOSITE_DFU_CDC)
	*length = usbd_os_str_desc.bLength;
	return (uint8_t *)&usbd_os_str_desc;
#else
	static uint8_t none[1] = "";
	*length = 0;
	return none;
#endif
}

static uint8_t *usbd_usr_ext_prop_feat_desc(uint8_t speed, uint16_t *length)
{
#if defined(USBD_CLASS_COMPOSITE_DFU_CDC)
	*length = usbd_winusb_ex_prop_desc.header.dwLength;
	return (uint8_t *)&usbd_winusb_ex_prop_desc;
#else
	static uint8_t none[1] = "";
	*length = 0;
	return none;
#endif
}

static uint8_t *usbd_usr_ext_compat_id_feat_desc(uint8_t speed, uint16_t *length)
{
#if defined(USBD_CLASS_COMPOSITE_DFU_CDC)
	*length = usbd_compat_id_desc.dwLength;
	return (uint8_t *)&usbd_compat_id_desc;
#else
	static uint8_t none[1] = "";
	*length = 0;
	return none;
#endif
}

uint8_t *usbd_get_cfg_desc(uint8_t speed, uint16_t *length)
{
	*length = sizeof(usbd_cfg_desc);
	return usbd_cfg_desc;
}

#if defined(USBD_CLASS_COMPOSITE_DFU_CDC) && defined(USE_STD_LIB_GEN)
static uint8_t hex2ch(uint8_t num) { return num > 9 ? num - 10 + 'A' : num + '0'; }
#endif

void usdb_desc_init(void)
{
#if defined(USBD_CLASS_COMPOSITE_DFU_CDC)
	// Generate GUID by PID/VID/NAME/SERIAL
#ifndef USE_STD_LIB_GEN
	uint8_t buf[64], hash[16];
	int len = snprintf((char *)buf, sizeof(buf), "USB\\VID_%04X&PID_%04X\\%s_%08lX%08lX%08lX", USBD_VID, USBD_PID, DEV, g_uid[0], g_uid[1], g_uid[2]);
	md5_data(buf, len, hash);
	len = snprintf((char *)buf, sizeof(buf), "{%02X%02X%02X%02X-%02X%02X-3%X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X}", // RFC9562 - Type 3
				   hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6] & 0xF, hash[7],
				   0x80 | (hash[8] & 0x3F), hash[9], hash[10], hash[11], hash[12], hash[13], hash[14], hash[15]);
	for(int i = 0; i < len && i < (int)(sizeof(usbd_winusb_ex_prop_desc.features[0].bPropertyData) / sizeof(usbd_winusb_ex_prop_desc.features[0].bPropertyData[0])); i++)
		usbd_winusb_ex_prop_desc.features[0].bPropertyData[i] = buf[i];
#else
	// smaller size without using libc
	uint8_t buf[64] = "USB\\VID_****&PID_****\\", hash[16];
	for(uint32_t i = 0; i < 4; i++)
		buf[8 + i] = hex2ch((USBD_VID >> (4 * (3 - i))) & 0xF);
	for(uint32_t i = 0; i < 4; i++)
		buf[17 + i] = hex2ch((USBD_PID >> (4 * (3 - i))) & 0xF);
	uint32_t len_dev_str = strlen(DEV);
	memcpy(buf + 22, DEV, len_dev_str);
	uint32_t c = 22 + len_dev_str + 1;
	buf[22 + len_dev_str] = '_';
	for(uint32_t i = 0; i < 3; i++)
		for(uint32_t j = 0; j < 8; j++)
			buf[c++] = hex2ch((g_uid[i] >> (4 * (7 - j))) & 0xF);
	md5_data(buf, c, hash);
	memcpy(buf, "{********-****-3***-****-************}", 38); // RFC9562 - Type 3
	buf[1] = hex2ch((hash[0] >> 4) & 0xF);
	buf[2] = hex2ch((hash[0] >> 0) & 0xF);
	buf[3] = hex2ch((hash[1] >> 4) & 0xF);
	buf[4] = hex2ch((hash[1] >> 0) & 0xF);
	buf[5] = hex2ch((hash[2] >> 4) & 0xF);
	buf[6] = hex2ch((hash[2] >> 0) & 0xF);
	buf[7] = hex2ch((hash[3] >> 4) & 0xF);
	buf[8] = hex2ch((hash[3] >> 0) & 0xF); // -
	buf[10] = hex2ch((hash[4] >> 4) & 0xF);
	buf[11] = hex2ch((hash[4] >> 0) & 0xF);
	buf[12] = hex2ch((hash[5] >> 4) & 0xF);
	buf[13] = hex2ch((hash[5] >> 0) & 0xF); // -
	buf[16] = hex2ch((hash[6] >> 0) & 0xF);
	buf[17] = hex2ch((hash[7] >> 4) & 0xF);
	buf[18] = hex2ch((hash[7] >> 0) & 0xF); // -
	buf[20] = hex2ch(((0x80 | (hash[8] & 0x30)) >> 4) & 0xF);
	buf[21] = hex2ch((hash[8] >> 0) & 0xF);
	buf[22] = hex2ch((hash[9] >> 4) & 0xF);
	buf[23] = hex2ch((hash[9] >> 0) & 0xF); // -
	buf[25] = hex2ch((hash[10] >> 4) & 0xF);
	buf[26] = hex2ch((hash[10] >> 0) & 0xF);
	buf[27] = hex2ch((hash[11] >> 4) & 0xF);
	buf[28] = hex2ch((hash[11] >> 0) & 0xF);
	buf[29] = hex2ch((hash[12] >> 4) & 0xF);
	buf[30] = hex2ch((hash[12] >> 0) & 0xF);
	buf[31] = hex2ch((hash[13] >> 4) & 0xF);
	buf[32] = hex2ch((hash[13] >> 0) & 0xF);
	buf[33] = hex2ch((hash[14] >> 4) & 0xF);
	buf[34] = hex2ch((hash[14] >> 0) & 0xF);
	buf[35] = hex2ch((hash[15] >> 4) & 0xF);
	buf[36] = hex2ch((hash[15] >> 0) & 0xF);
	for(int i = 0; i < 38 && i < (int)(sizeof(usbd_winusb_ex_prop_desc.features[0].bPropertyData) / sizeof(usbd_winusb_ex_prop_desc.features[0].bPropertyData[0])); i++)
		usbd_winusb_ex_prop_desc.features[0].bPropertyData[i] = buf[i];
#endif
#endif
}

USBD_DEVICE usbd_usr_cb = {
	usbd_usr_device_desc,
	usbd_usr_lang_id_desc,
	usbd_usr_manuf_desc,
	usbd_usr_prod_desc,
	usbd_usr_serial_desc,
	usbd_usr_cfg_desc,
	usbd_usr_if_desc,
	usbd_usr_os_str_desc,
	usbd_usr_ext_prop_feat_desc,
	usbd_usr_ext_compat_id_feat_desc,
};