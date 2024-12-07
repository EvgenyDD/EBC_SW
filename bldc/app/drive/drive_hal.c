#include "drive_hal.h"
#include "platform.h"

#define HALF_CORE_CLOCK (168000000 / 2)

void drive_init(void)
{
	uint32_t frequency = 15000;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure = {0};
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10; // PWM - H
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7; // HALL
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15; // BREAK + PWM - L
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_TIM1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_TIM1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource12, GPIO_AF_TIM1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_TIM1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_TIM1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_TIM1);

	// TIM1
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_Period = HALF_CORE_CLOCK / ((uint32_t)frequency) - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_CenterAligned2;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 1;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

	float dt = (float)((TIM1->PSC + 1) * (TIM1->ARR + 1)) / (float)HALF_CORE_CLOCK;

	TIM_OCInitTypeDef TIM_OCInitStructure = {0};
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_Pulse = TIM1->ARR / 2;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Set;

	TIM_OC1Init(TIM1, &TIM_OCInitStructure);
	TIM_OC2Init(TIM1, &TIM_OCInitStructure);
	TIM_OC3Init(TIM1, &TIM_OCInitStructure);

	TIM_BDTRInitTypeDef TIM_BDTRInitStruct = {0};
	TIM_BDTRInitStruct.TIM_OSSRState = TIM_OSSRState_Enable;
	TIM_BDTRInitStruct.TIM_OSSIState = TIM_OSSIState_Enable;
	TIM_BDTRInitStruct.TIM_LOCKLevel = TIM_LOCKLevel_OFF;
	TIM_BDTRInitStruct.TIM_DeadTime = 60;
	TIM_BDTRInitStruct.TIM_Break = TIM_Break_Enable;
	TIM_BDTRInitStruct.TIM_BreakPolarity = TIM_BreakPolarity_High;
	TIM_BDTRInitStruct.TIM_AutomaticOutput = TIM_AutomaticOutput_Enable;
	TIM_BDTRConfig(TIM1, &TIM_BDTRInitStruct);

	TIM_ARRPreloadConfig(TIM1, ENABLE);
	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_CCPreloadControl(TIM1, ENABLE);

	TIM_Cmd(TIM1, ENABLE);
	TIM_CtrlPWMOutputs(TIM1, ENABLE);
}