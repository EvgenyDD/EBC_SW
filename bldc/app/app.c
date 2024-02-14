#include "adc.h"
#include "config_system.h"
#include "crc.h"
#include "error.h"
#include "fw_header.h"
#include "platform.h"
#include "prof.h"
#include "ret_mem.h"
#include "usbd_proto_core.h"
#include <stdio.h>
#include <string.h>

int gsts = -10;

#define SYSTICK_IN_US (168000000 / 1000000)
#define SYSTICK_IN_MS (168000000 / 1000)

bool g_stay_in_boot = false;

uint32_t g_uid[3];

volatile uint64_t system_time = 0;
static int32_t prev_systick = 0;

config_entry_t g_device_config[] = {};
const uint32_t g_device_config_count = sizeof(g_device_config) / sizeof(g_device_config[0]);

void delay_ms(volatile uint32_t delay_ms)
{
	volatile uint32_t start = 0;
	int32_t mark_prev = 0;
	prof_mark(&mark_prev);
	const uint32_t time_limit = delay_ms * SYSTICK_IN_MS;
	for(;;)
	{
		start += (uint32_t)prof_mark(&mark_prev);
		if(start >= time_limit)
			return;
	}
}

static void end_loop(void)
{
	delay_ms(2000);
	platform_reset();
}

void main(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;

	platform_get_uid(g_uid);

	prof_init();
	platform_watchdog_init();

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA |
							   RCC_AHB1Periph_GPIOB |
							   RCC_AHB1Periph_GPIOC |
							   RCC_AHB1Periph_GPIOD,
						   ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_7; // DCDC_FAULT, SNS_KEY
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	fw_header_check_all();

	ret_mem_init();
	ret_mem_set_load_src(LOAD_SRC_APP); // let preboot know it was booted from bootloader

	adc_init();

	usb_init();

	if(config_validate() == CONFIG_STS_OK) config_read_storage();

	for(;;)
	{
		// time diff
		uint32_t time_diff_systick = (uint32_t)prof_mark(&prev_systick);

		static uint32_t remain_systick_us_prev = 0, remain_systick_ms_prev = 0;
		uint32_t diff_us = (time_diff_systick + remain_systick_us_prev) / (SYSTICK_IN_US);
		remain_systick_us_prev = (time_diff_systick + remain_systick_us_prev) % SYSTICK_IN_US;

		uint32_t diff_ms = (time_diff_systick + remain_systick_ms_prev) / (SYSTICK_IN_MS);
		remain_systick_ms_prev = (time_diff_systick + remain_systick_ms_prev) % SYSTICK_IN_MS;

		platform_watchdog_reset();

		system_time += diff_ms;

		adc_track();

		usb_poll(diff_ms);
	}
}