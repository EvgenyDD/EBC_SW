#ifndef RTC_H__
#define RTC_H__

#include "platform.h"
#include <stdint.h>

int rtc_init(void);
void mcu_rtc_get_date(RTC_DateTypeDef *RTC_DateStructure);
void mcu_rtc_get_time(RTC_TimeTypeDef *RTC_TimeStructure);

#endif // RTC_H__