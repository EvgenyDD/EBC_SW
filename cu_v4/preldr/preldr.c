#include "fw_header.h"
#include "platform.h"
#include "ret_mem.h"
#include "stm32f4xx.h"

extern int __preldr_start, __preldr_end;
extern int __ldr_start, __ldr_end;
extern int __app_start, __app_end;

// void SystemInit(void)
// {
// FLASH->ACR |= FLASH_ACR_LATENCY_2;
// RCC->CFGR |= RCC_CFGR_PPRE1_2;
// RCC->CFGR |= RCC_CFGR_PLLXTPRE_HSE;
// RCC->CR |= RCC_CR_HSEON;
// while(!(RCC->CR & RCC_CR_HSERDY))
// 	;

// RCC->CFGR |= RCC_CFGR_PLLSRC;
// RCC->CFGR |= RCC_CFGR_PLLMULL6;
// RCC->CR |= RCC_CR_PLLON;
// while(!(RCC->CR & RCC_CR_PLLRDY))
// 	;

// RCC->CFGR |= RCC_CFGR_SW_PLL;

// while(!(RCC->CFGR & RCC_CFGR_SWS_PLL))
// 	;

// FLASH->ACR |= FLASH_ACR_PRFTBE;

// RCC->AHBENR |= RCC_AHBENR_CRCEN;
// }

void main(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;

	ret_mem_init();

	// determine load source
	load_src_t load_src = ret_mem_get_load_src();
	ret_mem_set_load_src(LOAD_SRC_NONE);

	fw_header_check_all();

	// force goto app -> cause rebooted from bootloader
	if(load_src == LOAD_SRC_BOOTLOADER)
	{
		if(g_fw_info[FW_APP].locked == false)
		{
			platform_run_address((uint32_t)&__app_start);
		}
	}

	// run bootloader
	if(g_fw_info[FW_LDR].locked == false)
	{
		platform_run_address((uint32_t)&__ldr_start);
	}

	// load src not bootloader && bootloader is corrupt
	if(g_fw_info[FW_APP].locked == false)
	{
		platform_run_address((uint32_t)&__app_start);
	}

	while(1)
	{
	}
}