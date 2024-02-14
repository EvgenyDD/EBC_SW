#ifndef PROF_H_
#define PROF_H_

#include "stm32f4xx.h"

static inline void prof_init(void)
{
	// NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(3, 3, 0));
	// CLK_EnableSysTick(CLK_CLKSEL0_STCLKSEL_HXT, (CLK_GetHXTFreq() / 1000)); // Set 1ms tick
	SysTick->CTRL = 0UL;
	SysTick->LOAD = 0xffffffff;
	SysTick->VAL = 0;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
}

static inline int32_t prof_mark(int32_t *m)
{
	int32_t mark = (int32_t)((1 << 24) - SysTick->VAL - 1);
	int32_t prev = *m;
	*m = mark;
	return (mark - prev + (1 << 24)) & ((1 << 24) - 1);
}

#endif // PROF_H_