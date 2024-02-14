#include "stm32f4xx.h"

#define TIMEOUT 0xFFF
#define TIMEOUT_SHORT 100

#define _I2C_SR1_SB 0
#define _I2C_SR1_ADDR 1
#define _I2C_SR1_BTF 2
#define _I2C_SR1_ADD10 3
#define _I2C_SR1_STOPF 4
#define _I2C_SR1_RxNE 6
#define _I2C_SR1_TxE 7
#define _I2C_SR1_BERR 8
#define _I2C_SR1_ARLO 9
#define _I2C_SR1_AF 10
#define _I2C_SR1_OVR 11
#define _I2C_SR1_PECERR 12
#define _I2C_SR1_TIMEOUT 14
#define _I2C_SR1_SMBALERT 15

#define _I2C_SR2_MSL 0
#define _I2C_SR2_BUSY 1
#define _I2C_SR2_TRA 2
#define _I2C_SR2_GENCALL 4
#define _I2C_SR2_SMBDEFAULT 5
#define _I2C_SR2_SMBHOST 6
#define _I2C_SR2_DUALF 7
#define _I2C_SR2_PEC 8

#define _I2C_CR1_PE 0
#define _I2C_CR1_SMBUS 1
#define _I2C_CR1_SMBTYPE 3
#define _I2C_CR1_ENARP 4
#define _I2C_CR1_ENPEC 5
#define _I2C_CR1_ENGC 6
#define _I2C_CR1_NOSTRETCH 7
#define _I2C_CR1_START 8
#define _I2C_CR1_STOP 9
#define _I2C_CR1_ACK 10
#define _I2C_CR1_POS 11
#define _I2C_CR1_PEC 12
#define _I2C_CR1_ALERT 13
#define _I2C_CR1_SWRST 15

#define _I2C_SB_FLAG (1 << _I2C_SR1_SB)
#define _I2C_ADDR_FLAG (1 << _I2C_SR1_ADDR)
#define _I2C_BTF_FLAG (1 << _I2C_SR1_BTF)
// #define _I2C_ADD10_FLAG (1 << _I2C_SR1_ADD10)
// #define _I2C_STOPF_FLAG (1 << _I2C_SR1_STOPF)
#define _I2C_RxNE_FLAG (1 << _I2C_SR1_RxNE)
#define _I2C_TxE_FLAG (1 << _I2C_SR1_TxE)
// #define _I2C_BERR_FLAG (1 << _I2C_SR1_BERR)
// #define _I2C_ARLO_FLAG (1 << _I2C_SR1_ARLO)
// #define _I2C_AF_FLAG (1 << _I2C_SR1_AF)
// #define _I2C_OVR_FLAG (1 << _I2C_SR1_OVR)
// #define _I2C_PECERR_FLAG (1 << _I2C_SR1_PECERR)
// #define _I2C_TIMEOUT_FLAG (1 << _I2C_SR1_TIMEOUT)
// #define _I2C_SMBALERT_FLAG (1 << _I2C_SR1_SMBALERT)

static uint32_t time = 0;

#define CHK_EVT_RET(x, c)                      \
	for(time = TIMEOUT; time > 0 && x; time--) \
		;                                      \
	if(time == 0) return (c)

#define CHK_EVT_RET_SHORT(x, c)                      \
	for(time = TIMEOUT_SHORT; time > 0 && x; time--) \
		;                                            \
	if(time == 0) return (c)

static void Clear_ADDR_FLAG(void)
{
	volatile uint32_t DummyRead = 0;

	// check for device mode
	if((I2C1->SR2 & (1 << _I2C_SR2_MSL)))
	{
		// device is in transmit mode or don't know
		// clear the addr flag
		DummyRead = I2C1->SR1;
		DummyRead = I2C1->SR2;
		(void)DummyRead;
	}
	else
	{
		// device is in slave mode
		// clear the addr flag straight away
		DummyRead = I2C1->SR1;
		DummyRead = I2C1->SR2;
		(void)DummyRead;
	}
}

uint8_t _I2C_GetFlagStatus(uint32_t I2C_FLAG)
{
	if(I2C1->SR1 & I2C_FLAG)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void i2c_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	I2C_InitTypeDef I2C_InitStruct;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_I2C1);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	I2C_DeInit(I2C1);

	I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStruct.I2C_OwnAddress1 = 0xFE;
	I2C_InitStruct.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStruct.I2C_ClockSpeed = 400000;

	I2C_Init(I2C1, &I2C_InitStruct);

	I2C_Cmd(I2C1, ENABLE);
}

int i2c_read(uint8_t addr, uint8_t *data, uint16_t size, uint8_t rep_start)
{
	I2C1->CR1 |= (1 << _I2C_CR1_START);
	CHK_EVT_RET(!_I2C_GetFlagStatus(_I2C_SB_FLAG), -1);

	I2C1->DR = (addr << 1) | 1;
	CHK_EVT_RET(!_I2C_GetFlagStatus(_I2C_ADDR_FLAG), -2);

	if(size == 1)
	{
		I2C1->CR1 &= ~(1 << _I2C_CR1_ACK);
		Clear_ADDR_FLAG();
		CHK_EVT_RET(!_I2C_GetFlagStatus(_I2C_RxNE_FLAG), -3);
		if(!rep_start) I2C1->CR1 |= (1 << _I2C_CR1_STOP);
		*data = I2C1->DR;
	}
	if(size > 1)
	{
		Clear_ADDR_FLAG();
		for(uint32_t i = size; i > 0; i--)
		{
			CHK_EVT_RET(!_I2C_GetFlagStatus(_I2C_RxNE_FLAG), 3);
			if(i == 2)
			{
				I2C1->CR1 &= ~(1 << _I2C_CR1_ACK);
				if(!rep_start) I2C1->CR1 |= (1 << _I2C_CR1_STOP);
			}
			*data = I2C1->DR;
			data++;
		}
	}
	I2C1->CR1 |= (1 << _I2C_CR1_ACK); // enable the acking if ACK control enabled in module config

	return 0;
}

int i2c_write(uint8_t addr, const uint8_t *data, uint16_t size, uint8_t rep_start)
{
	I2C1->CR1 |= (1 << _I2C_CR1_START);
	CHK_EVT_RET(!_I2C_GetFlagStatus(_I2C_SB_FLAG), -1);

	I2C1->DR = addr << 1;
	CHK_EVT_RET(!_I2C_GetFlagStatus(_I2C_ADDR_FLAG), -2);

	Clear_ADDR_FLAG();

	if(size == 0) //!
	{
		CHK_EVT_RET(!_I2C_GetFlagStatus(_I2C_TxE_FLAG), -3);
		I2C1->DR = 0;
	}
	while(size > 0)
	{
		CHK_EVT_RET(!_I2C_GetFlagStatus(_I2C_TxE_FLAG), -4);
		I2C1->DR = *data;
		data++;
		size--;
	}

	CHK_EVT_RET(!_I2C_GetFlagStatus(_I2C_TxE_FLAG), -5);
	CHK_EVT_RET(!_I2C_GetFlagStatus(_I2C_BTF_FLAG), -6);

	if(!rep_start) I2C1->CR1 |= (1 << _I2C_CR1_STOP);

	return 0;
}
