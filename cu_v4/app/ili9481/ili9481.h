#ifndef ILI9481_H__
#define ILI9481_H__

#include <stdbool.h>
#include <stdint.h>

void ili9481_init(void);
void ili9481_poll(uint32_t diff_ms);

#endif // ILI9481_H__