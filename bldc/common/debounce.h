#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdbool.h>
#include <stdint.h>

#define BUTTON_THRS 30 // ms
#define BUTTON_PRESS_THRS (BUTTON_THRS * 9 / 10)
#define BUTTON_RELEASE_THRS (BUTTON_THRS * 1 / 10)

#define BTN_PRESS (1 << 0)
#define BTN_PRESS_LONG (1 << 1)
#define BTN_UNPRESS_SHOT (1 << 2)
#define BTN_PRESS_SHOT ((1 << 3) | BTN_PRESS)
#define BTN_PRESS_LONG_SHOT ((1 << 3) | BTN_PRESS_LONG)

typedef struct
{
	int state;
	int int_state;
	int int_state_prev;
	int cnt_fltr;
	int cnt_press;
	int cfg_release_thrs;
	int cfg_press_thrs;
	int cfg_max_thrs;
	int cfg_long_press_time;
} debounce_t;

void debounce_init(debounce_t *inst, int cfg_long_press_time);
void debounce_update(debounce_t *inst, bool state, int time_diff);

#endif // DEBOUNCE_H
