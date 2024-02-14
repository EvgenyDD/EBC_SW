#ifndef I2C_COMMON_H__
#define I2C_COMMON_H__

#include <stdint.h>

void i2c_init(void);
int i2c_write(uint8_t addr, const uint8_t *data, uint16_t size, uint8_t rep_start);
int i2c_read(uint8_t addr, uint8_t *data, uint16_t size, uint8_t rep_start);

#endif // I2C_COMMON_H__