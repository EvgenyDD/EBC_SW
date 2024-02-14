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

void SystemInit_ExtMemCtl(void)
{
	/* SRAM (LCD) configuration
	 +-------------------+--------------------+------------------+------------------+
	 | PD0  <-> FSMC_D2  | PE0  <!> FSMC_NBL0 | PF0  <!> FSMC_A0 | PG0 <!> FSMC_A10 |
	 | PD1  <-> FSMC_D3  | PE1  <!> FSMC_NBL1 | PF1  <!> FSMC_A1 | PG1 <!> FSMC_A11 |
	 | PD4  <-> FSMC_NOE | PE2  <-> FSMC_NE1  | PF2  <!> FSMC_A2 | PG2 <!> FSMC_A12 |
	 | PD5  <-> FSMC_NWE | PE3  <!> FSMC_A19  | PF3  <!> FSMC_A3 | PG3 <!> FSMC_A13 |
	 | PD8  <-> FSMC_D13 | PE4  <!> FSMC_A20  | PF4  <!> FSMC_A4 | PG4 <!> FSMC_A14 |
	 | PD9  <-> FSMC_D14 | PE7  <-> FSMC_D4   | PF5  <!> FSMC_A5 | PG5 <!> FSMC_A15 |
	 | PD10 <-> FSMC_D15 | PE8  <-> FSMC_D5   | PF12 <!> FSMC_A6 | PG9 <!> FSMC_NE2 |
	 | PD11 <-> FSMC_A16 | PE9  <-> FSMC_D6   | PF13 <!> FSMC_A7 |------------------+
	 | PD12 <!> FSMC_A17 | PE10 <-> FSMC_D7   | PF14 <!> FSMC_A8 |
	 | PD13 <!> FSMC_A18 | PE11 <-> FSMC_D8   | PF15 <!> FSMC_A9 |
	 | PD14 <-> FSMC_D0  | PE12 <-> FSMC_D9   |------------------+
	 | PD15 <-> FSMC_D1  | PE13 <-> FSMC_D10  |
	 |                   | PE14 <-> FSMC_D11  |
	 |                   | PE15 <-> FSMC_D12  |
	 +-------------------+--------------------+
	*/
	RCC->AHB1ENR = 0x00000018; // GPIOD, GPIOE clock

	// GPIOD->AFR[0] = 0x00cc0c0c; /* Connect PDx pins to FSMC Alternate function */
	// GPIOD->AFR[1] = 0xcc00cccc;
	// GPIOD->MODER = 0xa0aa0a0a;	 /* Configure PDx pins in Alternate function mode */
	// GPIOD->OSPEEDR = 0xf0ff0f0f; /* Configure PDx pins speed to 100 MHz */
	// GPIOD->OTYPER = 0x00000000;	 /* Configure PDx pins Output type to push-pull */
	// GPIOD->PUPDR = 0x00000000;	 /* No pull-up, pull-down for PDx pins */

	// GPIOE->AFR[0] = 0xc0000c00; /* Connect PEx pins to FSMC Alternate function */
	// GPIOE->AFR[1] = 0xcccccccc;
	// GPIOE->MODER = 0xaaaa8008;	 /* Configure PEx pins in Alternate function mode */
	// GPIOE->OSPEEDR = 0xffffc030; /* Configure PEx pins speed to 100 MHz */
	// GPIOE->OTYPER = 0x00000000;	 /* Configure PEx pins Output type to push-pull */
	// GPIOE->PUPDR = 0x00000000;	 /* No pull-up, pull-down for PEx pins */
	// #warning "here"
	GPIOD->AFR[0] = 0xc0cc00cc; /* Connect PDx pins to FSMC Alternate function */
	GPIOD->AFR[1] = 0xcc22cccc;
	GPIOD->MODER = 0xaaaa8a0a;	 /* Configure PDx pins in Alternate function mode */
	GPIOD->OSPEEDR = 0xf0ffcf0f; /* Configure PDx pins speed to 100 MHz */
	GPIOD->OTYPER = 0x00000000;	 /* Configure PDx pins Output type to push-pull */
	GPIOD->PUPDR = 0x00000000;	 /* No pull-up, pull-down for PDx pins */

	GPIOE->AFR[0] = 0xc3300000; /* Connect PEx pins to FSMC Alternate function */
	GPIOE->AFR[1] = 0xcccccccc;
	GPIOE->MODER = 0xaaaaa840;	 /* Configure PEx pins in Alternate function mode */
	GPIOE->OSPEEDR = 0xffffc000; /* Configure PEx pins speed to 100 MHz */
	GPIOE->OTYPER = 0x00000000;	 /* Configure PEx pins Output type to push-pull */
	GPIOE->PUPDR = 0x00000000;	 /* No pull-up, pull-down for PEx pins */

	RCC->AHB3ENR = 0x00000001; // FSMC interface clock

	/* Configure and enable Bank1_SRAM1 */
	FSMC_Bank1->BTCR[0] = 0x00001091; // BCR
	FSMC_Bank1->BTCR[1] =
		(0 << 16) | // bus turn
		(6 << 8) |	// data dur 1-255
		(1 << 4) |	// addr hold 1-15
		(5 << 0);	// addr setup 0-15
	FSMC_Bank1E->BWTR[2] = 0x0fffffff;
}

void SystemInit(void)
{
	SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2)); /* set CP10 and CP11 Full Access */

	clock_reinit();
	SystemInit_ExtMemCtl();
}
