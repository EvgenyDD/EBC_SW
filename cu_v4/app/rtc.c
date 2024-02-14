#include "rtc.h"
#include "platform.h"

int rtc_init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_AHB1Periph_BKPSRAM, ENABLE);
	PWR_BackupAccessCmd(ENABLE);

	if(RTC_ReadBackupRegister(RTC_BKP_DR1) != 0x55aa)
	{
		RCC_LSEConfig(RCC_LSE_ON);
		volatile uint32_t cnt = 0;
		while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
		{
			cnt++;
			if(cnt > 100000)
			{
				return 1;
			}
		}

		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
		RCC_RTCCLKCmd(ENABLE);

		RTC_InitTypeDef RTC_InitStructure;
		RTC_InitStructure.RTC_AsynchPrediv = 0x7F;
		RTC_InitStructure.RTC_SynchPrediv = 0xFF;
		RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
		RTC_Init(&RTC_InitStructure);

		RTC_TimeTypeDef RTC_TimeStructure;
		RTC_TimeStructure.RTC_H12 = RTC_H12_AM;
		RTC_TimeStructure.RTC_Hours = 0;
		RTC_TimeStructure.RTC_Minutes = 0;
		RTC_TimeStructure.RTC_Seconds = 0;
		RTC_SetTime(RTC_Format_BIN, &RTC_TimeStructure);

		RTC_DateTypeDef RTC_DateStructure;
		RTC_DateStructure.RTC_Date = 1;
		RTC_DateStructure.RTC_Month = RTC_Month_January;
		RTC_DateStructure.RTC_WeekDay = RTC_Weekday_Thursday;
		RTC_DateStructure.RTC_Year = 0;
		RTC_SetDate(RTC_Format_BIN, &RTC_DateStructure);

		RTC_WriteBackupRegister(RTC_BKP_DR1, 0x55aa);
	}
	return 0;
}

void mcu_rtc_get_date(RTC_DateTypeDef *RTC_DateStructure)
{
	RTC_GetDate(RTC_Format_BIN, RTC_DateStructure);
}

void mcu_rtc_get_time(RTC_TimeTypeDef *RTC_TimeStructure)
{
	RTC_GetTime(RTC_Format_BIN, RTC_TimeStructure);
}

void mcu_rtc_set_time(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
	RTC_TimeTypeDef RTC_TimeStructure;

	RTC_TimeStructure.RTC_H12 = RTC_H12_AM;
	RTC_TimeStructure.RTC_Hours = hours;
	RTC_TimeStructure.RTC_Minutes = minutes;
	RTC_TimeStructure.RTC_Seconds = seconds;
	RTC_SetTime(RTC_Format_BIN, &RTC_TimeStructure);
}

void mcu_rtc_set_date(uint8_t year, uint8_t weekday, uint8_t month, uint8_t date)
{
	RTC_DateTypeDef RTC_DateStructure;
	RTC_DateStructure.RTC_Date = date;
	RTC_DateStructure.RTC_Month = month;
	RTC_DateStructure.RTC_WeekDay = weekday;
	RTC_DateStructure.RTC_Year = year - 1970; // 已1970年为起点，
	RTC_SetDate(RTC_Format_BIN, &RTC_DateStructure);
}

void rtc_set_td(void)
{
	RTC_TimeTypeDef RTC_TimeStructure;

	RTC_TimeStructure.RTC_H12 = RTC_H12_AM;
	RTC_TimeStructure.RTC_Hours = 0;
	RTC_TimeStructure.RTC_Minutes = 0;
	RTC_TimeStructure.RTC_Seconds = 0;
	RTC_SetTime(RTC_Format_BIN, &RTC_TimeStructure);

	RTC_DateTypeDef RTC_DateStructure;
	RTC_DateStructure.RTC_Date = 1;
	RTC_DateStructure.RTC_Month = RTC_Month_January;
	RTC_DateStructure.RTC_WeekDay = RTC_Weekday_Thursday;
	RTC_DateStructure.RTC_Year = 0; // 1970
	RTC_SetDate(RTC_Format_BIN, &RTC_DateStructure);

	RTC_WriteBackupRegister(RTC_BKP_DR0, 0x55aa);
}
