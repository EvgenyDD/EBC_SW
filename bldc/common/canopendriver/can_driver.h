#ifndef CAN_DRIVER_H_
#define CAN_DRIVER_H_

/// CAN driver for STM32F3xx

#include "stm32f4xx.h"
#include <stdbool.h>

#define CAN_FILT_NUM 28
#define F_OSC_CAN 42000000

void can_drv_init(CAN_TypeDef *dev);
void can_drv_leave_init_mode(CAN_TypeDef *dev);
void can_drv_enter_init_mode(CAN_TypeDef *dev);
void can_drv_start(CAN_TypeDef *dev);
int32_t can_drv_check_set_bitrate(CAN_TypeDef *dev, int32_t bit_rate, bool is_set);
void can_drv_reset_module(CAN_TypeDef *dev);
void can_drv_set_rx_filter(CAN_TypeDef *dev, uint32_t filter, uint32_t id, uint32_t mask);
bool can_drv_check_rx_overrun(CAN_TypeDef *dev);
bool can_drv_check_bus_off(CAN_TypeDef *dev);
bool can_drv_is_rx_pending(CAN_TypeDef *dev);
uint16_t can_drv_get_rx_error_counter(CAN_TypeDef *dev);
uint16_t can_drv_get_tx_error_counter(CAN_TypeDef *dev);
int can_drv_get_rx_filter_index(CAN_TypeDef *dev);
uint32_t can_drv_get_rx_ident(CAN_TypeDef *dev);
int can_drv_get_rx_dlc(CAN_TypeDef *dev);
void can_drv_get_rx_data(CAN_TypeDef *dev, uint8_t *data);
void can_drv_release_rx_message(CAN_TypeDef *dev);
bool can_drv_tx(CAN_TypeDef *dev, uint32_t ident, uint8_t dlc, uint8_t *d);
bool can_drv_is_transmitting(CAN_TypeDef *dev);
uint32_t can_drv_is_message_sent(CAN_TypeDef *dev);
void can_drv_release_tx_message(CAN_TypeDef *dev, uint32_t mask);
void can_drv_tx_abort(CAN_TypeDef *dev);

#endif // CAN_DRIVER_H_
