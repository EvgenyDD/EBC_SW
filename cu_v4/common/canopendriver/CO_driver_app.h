#ifndef CO_DRIVER_H_
#define CO_DRIVER_H_

#include <stdint.h>

void co_od_init_headers(void);
void co_emcy_rcv_cb(const uint16_t ident, const uint16_t errorCode, const uint8_t errorRegister, const uint8_t errorBit, const uint32_t infoCode);
void co_emcy_print_hist(void);

#endif // CO_DRIVER_H_