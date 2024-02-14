#ifndef LSS_CB_H_
#define LSS_CB_H_

// LSS Callbacks

#include "CANopen.h"

typedef struct
{
	uint32_t lss_br_set_delay_counter; // us
	CO_t *co;
} LSS_cb_obj_t;

void lss_cb_init(LSS_cb_obj_t *obj);
void lss_cb_poll(LSS_cb_obj_t *obj, uint32_t diff_us);

#endif // LSS_CB_H_