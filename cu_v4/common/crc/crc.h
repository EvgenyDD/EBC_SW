#ifndef CRC_H_
#define CRC_H_

#include <stdint.h>

uint32_t crc32(const uint8_t *data, uint32_t size_bytes);
uint32_t crc32_start(const uint8_t *data, uint32_t size_bytes);
uint32_t crc32_end(const uint8_t *data, uint32_t size_bytes);

#endif // CRC_H_