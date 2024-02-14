#include "crc.h"
#include "stm32f4xx.h"
#include <string.h>

uint32_t crc32(const uint8_t *data, uint32_t size_bytes)
{
	CRC->CR = 1; // reset CRC module
	uint32_t w;
	for(uint32_t i = 0; i < (size_bytes >> 2); i++)
	{
		memcpy(&w, &data[i << 2], 4);
		CRC->DR = w;
	}
	return CRC->DR;
}

uint32_t crc32_start(const uint8_t *data, uint32_t size_bytes)
{
	CRC->CR = 1; // reset CRC module
	uint32_t w;
	for(uint32_t i = 0; i < (size_bytes >> 2); i++)
	{
		memcpy(&w, &data[i << 2], 4);
		CRC->DR = w;
	}
	return CRC->DR;
}

uint32_t crc32_end(const uint8_t *data, uint32_t size_bytes)
{
	uint32_t w;
	for(uint32_t i = 0; i < (size_bytes >> 2); i++)
	{
		memcpy(&w, &data[i << 2], 4);
		CRC->DR = w;
	}
	return CRC->DR;
}