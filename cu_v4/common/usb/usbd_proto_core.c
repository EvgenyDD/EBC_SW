#include "usbd_proto_core.h"
#include "fw_header.h"
#include "platform.h"
#include "ret_mem.h"
#include "usb_bsp.h"
#include "usb_conf.h"
#include "usb_dcd_int.h"
#include "usbd_desc.h"
#include "usbd_proto_vend.h"
#include "usbd_req.h"
#include "usbd_usr.h"
#include <string.h>

extern bool g_stay_in_boot;
extern void delay_ms(volatile uint32_t delay_ms);

#define DISC_TO_MS 100

#if FW_TYPE == FW_LDR
#define FW_TARGET FW_APP
#define ADDR_ORIGIN ((uint32_t) & __app_start)
#define ADDR_END ((uint32_t) & __app_end)
#elif FW_TYPE == FW_APP
#define FW_TARGET FW_LDR
#define ADDR_ORIGIN ((uint32_t) & __ldr_start)
#define ADDR_END ((uint32_t) & __ldr_end)
#endif

uint8_t usbd_buffer[USBD_BUF_SZ] = {0};
uint32_t usbd_buffer_rx_sz = 0;

uint8_t *USBD_DFU_GetCfgDesc(uint8_t speed, uint16_t *length);

static __IO uint32_t usbd_dfu_AltSet = 0;
static uint32_t reset_issue_cnt = 0;
static USB_OTG_CORE_HANDLE USB_OTG_dev;
static uint8_t fw_sts[3];
static volatile bool dl_pending = false;
static bool dwnload_was = false;

USBD_Usr_cb_TypeDef USR_cb = {
	USBD_USR_Init,
	USBD_USR_DeviceReset,
	USBD_USR_DeviceConfigured,
	USBD_USR_DeviceSuspended,
	USBD_USR_DeviceResumed,
	USBD_USR_DeviceConnected,
	USBD_USR_DeviceDisconnected,
};

void USBD_USR_Init(void) {}
void USBD_USR_DeviceReset(uint8_t speed) {}
void USBD_USR_DeviceConfigured(void) {}
void USBD_USR_DeviceSuspended(void) {}
void USBD_USR_DeviceResumed(void) {}
void USBD_USR_DeviceConnected(void) {}
void USBD_USR_DeviceDisconnected(void) {}

static uint8_t usbd_dfu_Init(void *pdev, uint8_t cfgidx) { return USBD_OK; }
static uint8_t usbd_dfu_DeInit(void *pdev, uint8_t cfgidx) { return USBD_OK; }

#if defined(USE_STM322xG_EVAL)
/* This value is set for SYSCLK = 120 MHZ, User can adjust this value
 * depending on used SYSCLK frequency */
#define count_us 40

#elif defined(USE_STM324xG_EVAL) || defined(USE_STM324x9I_EVAL)
/* This value is set for SYSCLK = 168 MHZ, User can adjust this value
 * depending on used SYSCLK frequency */
#define count_us 55

#else /* defined (USE_STM3210C_EVAL) */
/* This value is set for SYSCLK = 72 MHZ, User can adjust this value
 * depending on used SYSCLK frequency */
#define count_us 12
#endif
void USB_OTG_BSP_uDelay(const uint32_t usec)
{
	uint32_t count = count_us * usec;
	do
	{
		if(--count == 0)
		{
			return;
		}
	} while(1);
}

/**
 * @brief  USB_OTG_BSP_mDelay
 *          This function provides delay time in milli sec
 * @param  msec : Value of delay required in milli sec
 * @retval None
 */
void USB_OTG_BSP_mDelay(const uint32_t msec)
{
	delay_ms(msec);
}

static uint8_t usbd_dfu_Setup(void *pdev, USB_SETUP_REQ *req)
{
	switch(req->bmRequest & USB_REQ_TYPE_MASK)
	{
	case USB_REQ_TYPE_CLASS:
		switch(req->bRequest)
		{
		case DFU_DETACH:
			reset_issue_cnt = DISC_TO_MS;
			break;

		case DFU_GETSTATE:
		case DFU_CLRSTATUS:
			g_stay_in_boot = true;
			break;

		case DFU_GETSTATUS:
			g_stay_in_boot = true;
			fw_header_check_all();
			fw_sts[0] = g_fw_info[0].locked;
			fw_sts[1] = g_fw_info[1].locked;
			fw_sts[2] = g_fw_info[2].locked;
			USBD_CtlSendData(pdev, fw_sts, sizeof(fw_sts));
			break;

		case DFU_DNLOAD:
			if(req->wLength > sizeof(usbd_buffer) || req->wLength < 1 + 4)
			{
				USBD_CtlError(pdev, req);
				break;
			}
			dl_pending = true;
			usbd_buffer_rx_sz = req->wLength;
			USBD_CtlPrepareRx(pdev, usbd_buffer, req->wLength);
			break;

		default:
			USBD_CtlError(pdev, req);
			break;
		}
		break;

	case USB_REQ_TYPE_VENDOR:
		usbd_proto_vendor_handler(pdev, req);
		break;

	case USB_REQ_TYPE_STANDARD:
		switch(req->bRequest)
		{
		case USB_REQ_GET_DESCRIPTOR:
		{
			uint16_t len = 0;
			uint8_t *pbuf = NULL;
			if((req->wValue >> 8) == DFU_DESCRIPTOR_TYPE)
			{
				pbuf = USBD_DFU_GetCfgDesc(0, (uint16_t[]){0}) + 9 + (9 * USBD_ITF_MAX_NUM);
				len = MIN(USB_DFU_DESC_SIZ, req->wLength);
			}
			USBD_CtlSendData(pdev, pbuf, len);
		}
		break;

		case USB_REQ_GET_INTERFACE: USBD_CtlSendData(pdev, (uint8_t *)&usbd_dfu_AltSet, 1); break;

		case USB_REQ_SET_INTERFACE:
			if((uint8_t)(req->wValue) < USBD_ITF_MAX_NUM)
			{
				usbd_dfu_AltSet = (uint8_t)(req->wValue);
			}
			else
			{
				USBD_CtlError(pdev, req);
			}
			break;

		default: break;
		}
	default: break;
	}
	return USBD_OK;
}

static uint8_t EP0_TxSent(void *pdev)
{
	usbd_proto_vendor_tx_sent(pdev);
	return USBD_OK;
}

static uint8_t EP0_RxReady(void *pdev)
{
	if(dl_pending)
	{
		g_stay_in_boot = true;

		uint32_t addr_off, size_to_write = usbd_buffer_rx_sz - 4;
		memcpy(&addr_off, &usbd_buffer[0], 4);
		if(addr_off > ADDR_END - ADDR_ORIGIN || ADDR_END - ADDR_ORIGIN - size_to_write < addr_off)
		{
			dl_pending = false;
			return USBD_FAIL;
		}
		if(addr_off == 0) platform_flash_erase_flag_reset();
		int sts = platform_flash_write(ADDR_ORIGIN + addr_off, &usbd_buffer[4], size_to_write);
		dwnload_was = true;
		dl_pending = false;
		if(sts) return USBD_FAIL;
	}
	else
	{
		usbd_proto_vendor_rx_received(pdev);
	}
	return USBD_OK;
}

/* DFU interface class callbacks structure */
USBD_Class_cb_TypeDef DFU_cb = {
	usbd_dfu_Init,
	usbd_dfu_DeInit,
	usbd_dfu_Setup,
	EP0_TxSent,
	EP0_RxReady,
	NULL, /* DataIn, */
	NULL, /* DataOut, */
	NULL, /* SOF */
	NULL,
	NULL,
	USBD_DFU_GetCfgDesc,
};

void usb_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

#ifdef USE_STM3210C_EVAL
	RCC_OTGFSCLKConfig(RCC_OTGFSCLKSource_PLLVCO_Div3);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_OTG_FS, ENABLE);
#endif

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_OTG1_FS);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource12, GPIO_AF_OTG1_FS);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_OTG_FS, ENABLE);

	USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &DFU_cb, &USR_cb);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = OTG_FS_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	DCD_DevConnect(&USB_OTG_dev);
}

void usb_disconnect(void)
{
	DCD_DevDisconnect(&USB_OTG_dev);
	USB_OTG_StopDevice(&USB_OTG_dev);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void usb_poll(uint32_t diff_ms)
{
	if(reset_issue_cnt)
	{
		if(reset_issue_cnt <= diff_ms && usbd_proto_is_idle())
		{
			if(!dwnload_was) ret_mem_set_bl_stuck(true);
			platform_reset();
		}
		if(reset_issue_cnt > diff_ms) reset_issue_cnt -= diff_ms;
	}
}

void OTG_FS_IRQHandler(void) { USBD_OTG_ISR_Handler(&USB_OTG_dev); }
