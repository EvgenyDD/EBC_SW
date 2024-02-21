#include "ili9481.h"
#include "platform.h"
#include "stm32f4xx.h"
#include <stdlib.h>
#include <string.h>

#define LCD_BASE (0x60000000)
#define LCD_CMD (LCD_BASE)
#define LCD_DATA (LCD_BASE | (1 << (/*A*/ 16 + 1)))

#define LCD_ILI9481_WIDTH 480
#define LCD_ILI9481_HEIGHT 320

#define TFT_NOP 0x00
#define TFT_SWRST 0x01

#define TFT_SLPIN 0x10
#define TFT_SLPOUT 0x11

#define TFT_INVOFF 0x20
#define TFT_INVON 0x21

#define TFT_DISPOFF 0x28
#define TFT_DISPON 0x29

#define TFT_CASET 0x2A
#define TFT_PASET 0x2B
#define TFT_RAMWR 0x2C
#define TFT_RAMRD 0x2E

#define TFT_SCROLL_AREA 0x33
#define TFT_MADCTL 0x36
#define TFT_PIXEL_FMT 0x3A
#define TFT_FRM_MEM_ACC_IF_SETT 0xB3
#define TFT_PANEL_DRV 0xC0
#define TFT_TIMINGS_NORM 0xC1
#define TFT_TIMINGS_PART 0xC2
#define TFT_TIMINGS_IDLE 0xC3
#define TFT_FR_RATE_INV 0xC5
#define TFT_IFACE_CTRL 0xC6
#define TFT_GAMMA 0xC8
#define TFT_PWR_SETT 0xD0
#define TFT_VCOM 0xD1
#define TFT_PWR_SETT_NORM 0xD2
#define TFT_PWR_SETT_PART 0xD3
#define TFT_PWR_SETT_IDLE 0xD4
#define TFT_NV_MEM_WR 0xE0
#define TFT_NV_MEM_CTRL 0xE1
#define TFT_DEV_CODE 0xBF

volatile uint16_t *tft_cmd = (volatile uint16_t *)LCD_CMD;
volatile uint16_t *tft_data = (volatile uint16_t *)LCD_DATA;

static inline void writedata(uint16_t data) { *tft_data = data; }
static inline uint16_t readdata(void) { return *tft_data; }
static inline void writecommand(uint16_t com) { *tft_cmd = com; }

static bool dma_active = false;

#define DMA_STREAM DMA2_Stream1
#define DMA_CHANNEL DMA_Channel_1
#define DMA_STREAM_CLOCK RCC_AHB1Periph_DMA2
#define DMA_STREAM_IRQ DMA2_Stream1_IRQn
#define DMA_IT_TCIF DMA_IT_TCIF1
#define DMA_FLAG_TCIF DMA_FLAG_TCIF1
#define DMA_STREAM_IRQHANDLER DMA2_Stream1_IRQHandler

uint16_t convRGB565(uint8_t r, uint8_t g, uint8_t b);

static void ili9481_dma(uint16_t *data, uint32_t size)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	DMA_InitTypeDef DMA_InitStructure;
	RCC_AHB1PeriphClockCmd(DMA_STREAM_CLOCK, ENABLE);

	DMA_DeInit(DMA_STREAM);
	while(DMA_GetCmdStatus(DMA_STREAM) != DISABLE)
		;

	// NVIC_InitStructure.NVIC_IRQChannel = DMA_STREAM_IRQ;
	// NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	// NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	// NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	// NVIC_Init(&NVIC_InitStructure);

	DMA_InitStructure.DMA_Channel = DMA_CHANNEL;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)data;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)LCD_DATA;
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToMemory;
	DMA_InitStructure.DMA_BufferSize = size;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA_STREAM, &DMA_InitStructure);
	// DMA_ITConfig(DMA_STREAM, DMA_IT_TC, ENABLE);
	DMA_Cmd(DMA_STREAM, ENABLE);
	while(DMA_GetCmdStatus(DMA_STREAM) != ENABLE)
		;
	dma_active = true;
}

static void ili9481_setArea(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd)
{
	writecommand(TFT_CASET);
	writedata(xStart >> 8);
	writedata(xStart & 0xff);
	writedata(xEnd >> 8);
	writedata(xEnd & 0xff);

	writecommand(TFT_PASET);
	writedata(yStart >> 8);
	writedata(yStart & 0xff);
	writedata(yEnd >> 8);
	writedata(yEnd & 0xff);

	writecommand(TFT_RAMWR);
}

static void ili9481_refresh(bool new_frame, uint8_t phase, uint16_t *data, uint32_t size)
{
	if(new_frame) ili9481_setArea(0, 0, LCD_ILI9481_WIDTH - 1, LCD_ILI9481_HEIGHT - 1);
	ili9481_dma(data, size);
}

static void draw_pixel(uint32_t x, uint32_t y, uint16_t color)
{
	ili9481_setArea(x, y, x + 1, y + 1);
	writedata(color);
}

void ili9481_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init(GPIOE, &GPIO_InitStruct);

	GPIOE->BSRRH = GPIO_Pin_3;
	delay_ms(2);
	GPIOE->BSRRL = GPIO_Pin_3;
	delay_ms(11);

	volatile uint16_t id_buf[6];

	writecommand(TFT_DEV_CODE);
	for(uint32_t i = 0; i < sizeof(id_buf) / sizeof(id_buf[0]); i++)
	{
		id_buf[i] = readdata();
	}

	if(id_buf[1] != 2 &&
	   id_buf[2] != 4 &&
	   id_buf[3] != 0x94 &&
	   id_buf[4] != 0x81 &&
	   id_buf[5] != 0xff)
	{
		while(1)
		{
			GPIOA->BSRRL = 1 << 8;
			delay_ms(200);
			GPIOA->BSRRH = 1 << 8;
			delay_ms(200);
		}
	}

	writecommand(TFT_PWR_SETT);
	writedata(0x07);		   // 7 - 1.0xVci
	writedata((1 << 6) | 0x2); // BT  | 3
	writedata((1 << 4) | 0xB); // VRH | 5

	writecommand(TFT_VCOM);
	writedata(0);
	writedata(0x14); // VCM | 0
	writedata(0x1B); // VDV | 0

	writecommand(TFT_PWR_SETT_NORM);
	writedata(0x01);		 // AP        | 1
	writedata((1 << 4) | 2); // DC10 DC00 | 2

	writecommand(TFT_SLPOUT);
	delay_ms(120);

	writecommand(TFT_PANEL_DRV);
	writedata(0x10); // 0
	writedata(0x3B); // 3b
	writedata(0x00);
	writedata(0x02);
	writedata(0x01); // x11

	writecommand(TFT_FR_RATE_INV);
	writedata(0x03);

	writecommand(TFT_INVON);

	writecommand(TFT_FRM_MEM_ACC_IF_SETT);
	writedata(0x00);
	writedata(0x00);
	writedata(0x00);
	writedata(0x01);

	// writecommand(TFT_TIMINGS_NORM);
	// writedata(0x10);
	// writedata(0x10);
	// writedata(0x88);

	writecommand(TFT_GAMMA);
#if 1
	writedata(0x00);
	writedata(0x46);
	writedata(0x44);
	writedata(0x50);
	writedata(0x04);
	writedata(0x16);
	writedata(0x33);
	writedata(0x13);
	writedata(0x77);
	writedata(0x05);
	writedata(0x0F);
	writedata(0x00);
#else
	writedata(0x00);
	writedata(0x32);
	writedata(0x36);
	writedata(0x45);
	writedata(0x06);
	writedata(0x16);
	writedata(0x37);
	writedata(0x75);
	writedata(0x77);
	writedata(0x54);
	writedata(0x0C);
	writedata(0x00);
#endif

	writecommand(TFT_TIMINGS_IDLE);
	writedata(0x10);
	writedata(0x20);
	writedata(0x08);

	writecommand(TFT_MADCTL);
	writedata((1 << 5) | (1 << 3) | (1 << 0));

	writecommand(TFT_PIXEL_FMT);
	writedata(0x55); // 16-bit colour interface

	writecommand(TFT_DISPON);

	GPIOA->BSRRL = GPIO_Pin_8;

	for(uint32_t x = 100; x < 200; x++)
		for(uint32_t y = 150; y < 250; y++)
			draw_pixel(x, y, /*0x1F*/ convRGB565(255, 0, 0));
}

uint16_t fill_data[LCD_ILI9481_WIDTH * LCD_ILI9481_HEIGHT / 4];

static uint32_t phase = 0;
uint8_t rc = 0;

static void memset_color(uint16_t *data, uint16_t color, uint32_t size_bytes)
{
	for(uint32_t i = 0; i < size_bytes >> 1; i++)
	{
		data[i] = color;
	}
}

void ili9481_poll(uint32_t diff_ms)
{
	if(DMA_GetFlagStatus(DMA_STREAM, DMA_FLAG_TCIF))
	{
		DMA_ClearFlag(DMA_STREAM, DMA_FLAG_TCIF);
		dma_active = false;
		phase++;
	}
	if(dma_active) return;
#define vx 4
	if(phase < vx) ili9481_refresh(phase == 0, phase, fill_data, LCD_ILI9481_WIDTH * LCD_ILI9481_HEIGHT / 4);

	static uint32_t t = 0;
	t += diff_ms;
	if(t <= 500)
	{
		if(phase == 4)
		{
			for(uint32_t x = 100; x < 200; x++)
				for(uint32_t y = 50; y < 250; y++)
					draw_pixel(x, y, /*0x1F*/ convRGB565(255, 0, 0));
			for(uint32_t x = 200; x < 300; x++)
				for(uint32_t y = 100; y < 250; y++)
					draw_pixel(x, y, /*0x7e0*/ convRGB565(0, 255, 0));
			for(uint32_t x = 300; x < 400; x++)
				for(uint32_t y = 150; y < 250; y++)
					draw_pixel(x, y, /*0xF800*/ convRGB565(0, 0, 255));
			phase++;
		}
		return;
	}
	t = 0;
	if(phase >= vx)
	{
		if(++rc >= 8) rc = 0;
		memset_color(fill_data, convRGB565((rc & 1) ? 255 : 0, (rc & 2) ? 255 : 0, (rc & 4) ? 255 : 0), sizeof(fill_data));
		phase = 0;
	}
}

uint16_t convRGB565(uint8_t r, uint8_t g, uint8_t b)
{
	r >>= 3;
	g >>= 2;
	b >>= 3;
	uint8_t dataHigh = r << 3 | g >> 3;
	uint8_t dataLow = (g & 0x07) << 5 | b;
	return dataHigh << 8 | dataLow;
}