#ifndef ANALOG_H__
#define ANALOG_H__

#include <stdint.h>
typedef struct
{
	float vin, vp24, vm24, i24, t[4], t_mcu;
} adc_val_t;

void adc_init(void);
void adc_track(void);

extern adc_val_t adc_val;

#endif // ANALOG_H__