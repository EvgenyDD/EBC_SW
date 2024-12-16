#include "can_driver.h"
#include <string.h>

#define _INAK_TIMEOUT ((uint32_t)0x00FFFFFF)

void can_drv_init(CAN_TypeDef *dev)
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_CAN1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_CAN1);

	/* Configure CAN RX and TX pins */
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void can_drv_leave_init_mode(CAN_TypeDef *dev)
{
	uint32_t reg = dev->BTR;
	reg &= ~CAN_BTR_SILM;
	reg &= ~CAN_BTR_LBKM;
	dev->BTR = reg;
	dev->MCR &= (uint32_t)~CAN_MCR_INRQ;
	while((dev->MSR & CAN_MSR_INAK) != 0U)
		;
}

void can_drv_enter_init_mode(CAN_TypeDef *dev)
{
	dev->MCR |= CAN_MCR_INRQ;
	while((dev->MSR & CAN_MSR_INAK) == 0U)
		;
}

void can_drv_start(CAN_TypeDef *dev)
{
	dev->IER |= CAN_IER_LECIE | CAN_IER_BOFIE | CAN_IER_EPVIE | CAN_IER_EWGIE;

	dev->MCR &= (uint32_t)~CAN_MCR_NART;  // disable no auto-retransmit
	dev->MCR |= CAN_MCR_ABOM;			  // enable bus off recovery
	dev->MCR |= CAN_MCR_AWUM;			  // enable sleep recovery
	dev->MCR |= CAN_MCR_TXFP;			  // transmit order chronologically
	dev->MCR &= (uint32_t)~CAN_MCR_SLEEP; // disable sleep mode

	SET_BIT(dev->FMR, CAN_FMR_FINIT);

	dev->FA1R = 0; // deactivate all filters
	// assign all filters to master CAN (CAN1, CAN3, etc.)
	// CLEAR_BIT(dev->FMR, CAN_FMR_CAN2SB);
	// SET_BIT(dev->FMR, CAN_FILT_NUM << CAN_FMR_CAN2SB_Pos);
	dev->FM1R = 0;			 // all filters in mask mode
	dev->FS1R = 0x0fffffffu; // all filters in single 32bit mode
	dev->FFA1R = 0x0u;		 // assign all filters to FIFO0

	// clear all IDs & masks
	for(int i = 0; i < CAN_FILT_NUM; i++)
	{
		dev->sFilterRegister[i].FR1 = 0;
		dev->sFilterRegister[i].FR2 = 0;
	}

	CLEAR_BIT(dev->FMR, CAN_FMR_FINIT);
}

int32_t can_drv_check_set_bitrate(CAN_TypeDef *dev, int32_t bit_rate, bool is_set)
{
	int32_t res = 0;

	do
	{
		int32_t tqs, brp;
		for(brp = 1; brp < 0x400; brp++)
		{
			tqs = F_OSC_CAN / brp / bit_rate;
			if(tqs >= 5 && tqs <= 16 && (F_OSC_CAN / brp / tqs == bit_rate)) break;
		}
		if(brp >= 0x400) break;

		int32_t tseg1, tseg2;
		tseg1 = tseg2 = (tqs - 1) / 2;
		if(tqs - (1 + tseg1 + tseg2) == 1)
		{
			tseg1++;
		}

		if(tseg2 > 8)
		{
			tseg1 += tseg2 - 8;
			tseg2 = 8;
		}

		if(is_set)
		{
			uint32_t reg = dev->BTR;
			reg &= ~(0x3ffU | (0x7fU << 16U));
			reg |= (uint32_t)(brp - 1) | ((uint32_t)(tseg1 - 1) << 16U) | ((uint32_t)(tseg2 - 1) << 20U);
			dev->BTR = reg;
		}

		res = bit_rate;
	} while(0);

	return res;
}

void can_drv_reset_module(CAN_TypeDef *dev)
{
	SET_BIT(dev->MCR, CAN_MCR_RESET);
	while(dev->MCR & CAN_MCR_RESET)
		;
}

void can_drv_set_rx_filter(CAN_TypeDef *dev, uint32_t filter, uint32_t id, uint32_t mask)
{
	SET_BIT(dev->FMR, CAN_FMR_FINIT);
	CLEAR_BIT(dev->FA1R, 1U << filter);
	dev->sFilterRegister[filter].FR1 = id;
	dev->sFilterRegister[filter].FR2 = mask;
	CLEAR_BIT(dev->FM1R, 1U << filter);
	CLEAR_BIT(dev->FFA1R, 1U << filter);
	SET_BIT(dev->FA1R, 1U << filter);
	CLEAR_BIT(dev->FMR, CAN_FMR_FINIT);
}

bool can_drv_check_rx_overrun(CAN_TypeDef *dev)
{
	if(dev->RF0R & CAN_RF0R_FOVR0)
	{
		dev->RF0R |= CAN_RF0R_FOVR0;
		return true;
	}

	return false;
}

bool can_drv_check_bus_off(CAN_TypeDef *dev)
{
	if(dev->ESR & CAN_ESR_BOFF)
	{
		dev->ESR &= ~CAN_ESR_BOFF;
		return true;
	}
	return false;
}

bool can_drv_is_rx_pending(CAN_TypeDef *dev) { return ((dev->RF0R & CAN_RF0R_FMP0) > 0); }
uint16_t can_drv_get_rx_error_counter(CAN_TypeDef *dev) { return CAN_GetReceiveErrorCounter(dev); }
uint16_t can_drv_get_tx_error_counter(CAN_TypeDef *dev) { return CAN_GetLSBTransmitErrorCounter(dev); }
int can_drv_get_rx_filter_index(CAN_TypeDef *dev) { return (dev->sFIFOMailBox[0].RDTR >> 8) & 0xff; }
uint32_t can_drv_get_rx_ident(CAN_TypeDef *dev) { return dev->sFIFOMailBox[0].RIR; }
int can_drv_get_rx_dlc(CAN_TypeDef *dev) { return dev->sFIFOMailBox[0].RDTR & 0xff; }

inline static void memcpy_volatile(volatile void *dst, const volatile void *src, size_t size)
{
	for(uint32_t i = 0; i < size; i++)
	{
		*((volatile uint8_t *)dst + i) = *((const volatile uint8_t *)src + i);
	}
}

void can_drv_get_rx_data(CAN_TypeDef *dev, uint8_t *data)
{
	memcpy_volatile(data, &dev->sFIFOMailBox[0].RDLR, 8);
}

void can_drv_release_rx_message(CAN_TypeDef *dev) { dev->RF0R = CAN_RF0R_RFOM0; }

bool can_drv_tx(CAN_TypeDef *dev, uint32_t ident, uint8_t dlc, uint8_t *d)
{
	uint32_t txmb;
	if((dev->TSR & CAN_TSR_TME0) == CAN_TSR_TME0)
	{
		txmb = 0U;
	}
	else if((dev->TSR & CAN_TSR_TME1) == CAN_TSR_TME1)
	{
		txmb = 1U;
	}
	else if((dev->TSR & CAN_TSR_TME2) == CAN_TSR_TME2)
	{
		txmb = 2U;
	}
	else
	{
		return false;
	}

	dev->sTxMailBox[txmb].TIR &= CAN_TI0R_TXRQ;
	dev->sTxMailBox[txmb].TIR = ident;

	dev->sTxMailBox[txmb].TDTR &= 0xFFFFFFF0U;
	dev->sTxMailBox[txmb].TDTR |= dlc;

	dev->sTxMailBox[txmb].TDLR = (((uint32_t)d[3] << 24U) |
								  ((uint32_t)d[2] << 16U) |
								  ((uint32_t)d[1] << 8U) |
								  ((uint32_t)d[0]));
	dev->sTxMailBox[txmb].TDHR = (((uint32_t)d[7] << 24U) |
								  ((uint32_t)d[6] << 16U) |
								  ((uint32_t)d[5] << 8U) |
								  ((uint32_t)d[4]));

	dev->sTxMailBox[txmb].TIR |= CAN_TI0R_TXRQ;

	return true;
}

bool can_drv_is_transmitting(CAN_TypeDef *dev)
{
	return (dev->TSR & (CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2)) !=
		   (CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2);
}

uint32_t can_drv_is_message_sent(CAN_TypeDef *dev)
{
	if(dev->TSR & CAN_TSR_RQCP0) return dev->TSR & CAN_TSR_RQCP0;
	if(dev->TSR & CAN_TSR_RQCP1) return dev->TSR & CAN_TSR_RQCP1;
	if(dev->TSR & CAN_TSR_RQCP2) return dev->TSR & CAN_TSR_RQCP2;
	return 0;
}

void can_drv_release_tx_message(CAN_TypeDef *dev, uint32_t mask) { dev->TSR |= mask; }

void can_drv_tx_abort(CAN_TypeDef *dev)
{
	dev->TSR |= CAN_TSR_ABRQ0 | CAN_TSR_ABRQ1 | CAN_TSR_ABRQ2;
	while(dev->TSR & (CAN_TSR_ABRQ0 | CAN_TSR_ABRQ1 | CAN_TSR_ABRQ2))
		;
}