
#include "ret_mem.h"
#include "stm32f4xx.h"

void ret_mem_init(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
	RCC->BDCR |= RCC_BDCR_RTCEN;
	PWR->CR |= PWR_CR_DBP;
	while((PWR->CR & PWR_CR_DBP) == 0)
		;
}

load_src_t ret_mem_get_load_src(void)
{
	return RTC->BKP0R;
}

void ret_mem_set_load_src(load_src_t src)
{
	RTC->BKP0R = src;
}


void ret_mem_set_bl_stuck(bool state)
{
	RTC->BKP1R = state ? 0xAA : 0;
}

int ret_mem_is_bl_stuck(void)
{
	return RTC->BKP1R == 0xAA;
}