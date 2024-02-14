#include "adc.h"
#include "stm32f4xx.h"

#define SAMPLE_ACC_COUNT 128

#define VREFINT_CAL_ADDR 0x1FFF7A2A
#define TS_CAL1_ADDR 0x1FFF7A2C
#define TS_CAL2_ADDR 0x1FFF7A2E

enum
{
	ADC_CH_BRK1 = 0,
	ADC_CH_ACC,
	ADC_CH_SRV,
	ADC_CH_BRK0,
	ADC_CH_VUSB,
	ADC_CH_VBUS,
	ADC_CH_TEMP_NTC,
	ADC_CH_T_MCU,
	ADC_CH_VREFINT,
	ADC_CH,
};

adc_val_t adc_val = {0};

static volatile uint16_t adc_buf[ADC_CH];
static volatile uint32_t adc_buf_acc[ADC_CH][SAMPLE_ACC_COUNT] = {0};
static volatile uint32_t adc_buf_ptr = 0;

#define NTC_COEF 3380.0f
// NTC -> lo side
#define NTC_RES_LO_SIDE(adc_val) ((adc_val * 10000.0f) / (4095 - adc_val))
#define NTC_TEMP_LO_SIDE(adc_raw, coef) (1.0f / ((logf_fast(NTC_RES_LO_SIDE(adc_raw) / 10000.0f) / coef) + (1.0 / 298.15f)) - 273.15f)

static float adcVREFINTCAL, adcTSCAL1, adcTSSlopeK;

static inline int __float_as_int(float in)
{
	union fi
	{
		int i;
		float f;
	} conv;
	conv.f = in;
	return conv.i;
}

static inline float __int_as_float(int in)
{
	union fi
	{
		int i;
		float f;
	} conv;
	conv.i = in;
	return conv.f;
}

/* natural log on [0x1.f7a5ecp-127, 0x1.fffffep127]. Maximum relative error 9.4529e-5 */
static inline float logf_fast(float a)
{
	float m, r, s, t, i, f;
	int32_t e;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
	e = (__float_as_int(a) - 0x3f2aaaab) & 0xff800000;
#pragma GCC diagnostic pop

	m = __int_as_float(__float_as_int(a) - e);
	i = (float)e * 1.19209290e-7f; // 0x1.0p-23
	/* m in [2/3, 4/3] */
	f = m - 1.0f;
	s = f * f;
	/* Compute log1p(f) for f in [-1/3, 1/3] */
	r = (0.230836749f * f - 0.279208571f); // 0x1.d8c0f0p-3, -0x1.1de8dap-2
	t = (0.331826031f * f - 0.498910338f); // 0x1.53ca34p-2, -0x1.fee25ap-2
	r = (r * s + t);
	r = (r * s + f);
	r = (i * 0.693147182f + r); // 0x1.62e430p-1 // log(2)
	return r;
}

void adc_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	DMA_DeInit(DMA2_Stream0);
	DMA_InitTypeDef DMA_InitStructure;
	DMA_InitStructure.DMA_Channel = DMA_Channel_0;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)adc_buf;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_BufferSize = ADC_CH;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA2_Stream0, &DMA_InitStructure);
	DMA_Cmd(DMA2_Stream0, ENABLE);

	ADC_DeInit();

	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);

	ADC_InitTypeDef ADC_InitStructure;
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion = ADC_CH;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_480Cycles);	 // PA0
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_480Cycles);	 // PA1
	ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 3, ADC_SampleTime_480Cycles);	 // PA2
	ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 4, ADC_SampleTime_480Cycles);	 // PA3
	ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 5, ADC_SampleTime_480Cycles); // PC1
	ADC_RegularChannelConfig(ADC1, ADC_Channel_12, 6, ADC_SampleTime_480Cycles); // PC2
	ADC_RegularChannelConfig(ADC1, ADC_Channel_13, 7, ADC_SampleTime_480Cycles); // PC3
	ADC_RegularChannelConfig(ADC1, ADC_Channel_TempSensor, 8, ADC_SampleTime_480Cycles);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_Vrefint, 9, ADC_SampleTime_480Cycles);

	ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);

	ADC_DMACmd(ADC1, ENABLE);

	ADC_Cmd(ADC1, ENABLE);
	ADC_ContinuousModeCmd(ADC1, ENABLE);

	ADC_TempSensorVrefintCmd(ENABLE);

	ADC_SoftwareStartConv(ADC1);

	adcVREFINTCAL = *(uint16_t *)VREFINT_CAL_ADDR;
	adcTSCAL1 = *(uint16_t *)TS_CAL1_ADDR;
	float adcTSCAL2 = *(uint16_t *)TS_CAL2_ADDR;
	adcTSSlopeK = (110.0f - 30.0f) / (adcTSCAL2 - adcTSCAL1);
}

void adc_track(void)
{
	volatile uint32_t buf_acc[ADC_CH] = {0};

	for(uint32_t i = 0; i < ADC_CH; i++)
	{
		adc_buf_acc[i][adc_buf_ptr] = adc_buf[i];
		for(uint32_t j = 0; j < SAMPLE_ACC_COUNT; j++)
		{
			buf_acc[i] += adc_buf_acc[i][j];
		}
		buf_acc[i] /= SAMPLE_ACC_COUNT;
	}
	if(++adc_buf_ptr >= SAMPLE_ACC_COUNT) adc_buf_ptr = 0;

	adc_val.brk[0] = buf_acc[ADC_CH_BRK0];
	adc_val.brk[1] = buf_acc[ADC_CH_BRK1];
	adc_val.acc = buf_acc[ADC_CH_ACC];
	adc_val.srv = buf_acc[ADC_CH_SRV];
	adc_val.vusb = (float)buf_acc[ADC_CH_VUSB] / 4095.0f * 3.3f * (1.0f + 100.0f / 10.0f);
	adc_val.vbus = (float)buf_acc[ADC_CH_VBUS] / 4095.0f * 3.3f * (1.0f + 100.0f / 3.0f);
	adc_val.t_ntc = NTC_TEMP_LO_SIDE(buf_acc[ADC_CH_TEMP_NTC], NTC_COEF);
	adc_val.t_mcu = ((float)buf_acc[ADC_CH_T_MCU] * adcVREFINTCAL / (float)buf_acc[ADC_CH_VREFINT] - adcTSCAL1) * adcTSSlopeK + 30.0f;
}
