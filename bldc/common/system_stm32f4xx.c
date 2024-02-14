#include "stm32f4xx.h"

#if !defined(HSE_VALUE)
#define HSE_VALUE ((uint32_t)12000000) /*!< Default value of the External oscillator in Hz */
#endif								   /* HSE_VALUE */

#if !defined(HSI_VALUE)
#define HSI_VALUE ((uint32_t)16000000) /*!< Value of the Internal oscillator in Hz*/
#endif								   /* HSI_VALUE */

/************************* Miscellaneous Configuration ************************/
/*!< Uncomment the following line if you need to use external SRAM or SDRAM as data memory  */
#if defined(STM32F405xx) || defined(STM32F415xx) || defined(STM32F407xx) || defined(STM32F417xx) || defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx) || defined(STM32F469xx) || defined(STM32F479xx) || defined(STM32F412Zx) || defined(STM32F412Vx)
#define DATA_IN_ExtSRAM
#endif /* STM32F40xxx || STM32F41xxx || STM32F42xxx || STM32F43xxx || STM32F469xx || STM32F479xx || \
			STM32F412Zx || STM32F412Vx */

#if defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx) || defined(STM32F446xx) || defined(STM32F469xx) || defined(STM32F479xx)
/* #define DATA_IN_ExtSDRAM */
#endif /* STM32F427xx || STM32F437xx || STM32F429xx || STM32F439xx || STM32F446xx || STM32F469xx || \
			STM32F479xx */

/* Note: Following vector table addresses must be defined in line with linker
		 configuration. */
/*!< Uncomment the following line if you need to relocate the vector table
	 anywhere in Flash or Sram, else the vector table is kept at the automatic
	 remap of boot address selected */
/* #define USER_VECT_TAB_ADDRESS */

#if defined(USER_VECT_TAB_ADDRESS)
/*!< Uncomment the following line if you need to relocate your vector Table
	 in Sram else user remap will be done in Flash. */
/* #define VECT_TAB_SRAM */
#if defined(VECT_TAB_SRAM)
#define VECT_TAB_BASE_ADDRESS SRAM_BASE /*!< Vector Table base address field. \
											   This value must be a multiple of 0x200. */
#define VECT_TAB_OFFSET 0x00000000U		/*!< Vector Table base offset field. \
											   This value must be a multiple of 0x200. */
#else
#define VECT_TAB_BASE_ADDRESS FLASH_BASE /*!< Vector Table base address field. \
												This value must be a multiple of 0x200. */
#define VECT_TAB_OFFSET 0x00000000U		 /*!< Vector Table base offset field. \
												This value must be a multiple of 0x200. */
#endif									 /* VECT_TAB_SRAM */
#endif									 /* USER_VECT_TAB_ADDRESS */
/******************************************************************************/

uint32_t SystemCoreClock = 168000000;
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4};

void clock_reinit(void)
{
	RCC_DeInit();
	RCC_HSEConfig(RCC_HSE_ON);
	ErrorStatus errorStatus = RCC_WaitForHSEStartUp();
	RCC_LSICmd(ENABLE);
	while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
		;

	if(errorStatus == SUCCESS)
	{
		RCC_PLLConfig(RCC_PLLSource_HSE, 6, 168, 2, 7);
		RCC->CR |= RCC_CR_CSSON;
		RCC_HSICmd(DISABLE);
	}
	else
	{
		RCC_PLLConfig(RCC_PLLSource_HSI, 8, 168, 2, 7);
		RCC->CR &= ~(RCC_CR_CSSON);
	}

	RCC_PLLCmd(ENABLE);
	while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
		;

	FLASH_SetLatency(FLASH_Latency_5);

	RCC_HCLKConfig(RCC_SYSCLK_Div1);
	RCC_PCLK1Config(RCC_HCLK_Div4);
	RCC_PCLK2Config(RCC_HCLK_Div2);
	RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
}

void NMI_Handler(void)
{
	RCC->CIR |= RCC_CIR_CSSC;
	clock_reinit();
}

void SystemInit(void)
{
	SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2)); /* set CP10 and CP11 Full Access */

	clock_reinit();
}
