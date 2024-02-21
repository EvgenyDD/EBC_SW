#include "fw_header.h"
#include "platform.h"
#include "ret_mem.h"
#include "stm32f4xx.h"

__attribute__((noreturn)) void main(void)
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