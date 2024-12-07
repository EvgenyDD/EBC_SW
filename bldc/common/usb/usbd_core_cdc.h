#ifndef USBD_CORE_CDC_H__
#define USBD_CORE_CDC_H__

#include "usbd_ioreq.h"

// CDC Requests
#define SEND_ENCAPSULATED_COMMAND 0x00
#define GET_ENCAPSULATED_RESPONSE 0x01
#define SET_COMM_FEATURE 0x02
#define GET_COMM_FEATURE 0x03
#define CLEAR_COMM_FEATURE 0x04
#define SET_LINE_CODING 0x20
#define GET_LINE_CODING 0x21
#define SET_CONTROL_LINE_STATE 0x22
#define SEND_BREAK 0x23
#define NO_CMD 0xFF

typedef struct
{
	uint32_t bitrate;
	uint8_t format;
	uint8_t paritytype;
	uint8_t datatype;
} LINE_CODING;

uint8_t usbd_cdc_init(void *pdev, uint8_t cfgidx);
uint8_t usbd_cdc_deinit(void *pdev, uint8_t cfgidx);
void usb_cdc_restore_desc_composite(void);
uint8_t usbd_cdc_setup(void *pdev, USB_SETUP_REQ *req);
uint8_t usbd_cdc_data_in(void *pdev, uint8_t epnum);
uint8_t usbd_cdc_data_out(void *pdev, uint8_t epnum);
uint8_t usbd_cdc_sof(void *pdev);
uint8_t usbd_cdc_ep0_rx_ready(void *pdev);

int usbd_cdc_push_data(const uint8_t *data, uint32_t size);
extern void usbd_cdc_rx(const uint8_t *data, uint32_t size);

void usbd_cdc_lock(void);
void usbd_cdc_unlock(void);

#endif // USBD_CORE_CDC_H__
