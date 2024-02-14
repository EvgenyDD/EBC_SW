#include "debounce.h"
#include <string.h>

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define CLIP(a, l, h) (MAX((MIN((a), (h))), (l)))

void debounce_init(debounce_t *b, int cfg_long_press_time)
{
	memset(b, 0, sizeof(b));
	b->cfg_max_thrs = BUTTON_THRS;
	b->cfg_release_thrs = BUTTON_RELEASE_THRS;
	b->cfg_press_thrs = BUTTON_PRESS_THRS;
	b->cfg_long_press_time = cfg_long_press_time;
}

void debounce_update(debounce_t *b, bool state, int time_diff)
{
	if(time_diff != 0)
	{
		b->cnt_fltr += (state ? 1 : -1) * time_diff;
		b->cnt_fltr = CLIP(b->cnt_fltr, 0, b->cfg_max_thrs);

		if(b->cnt_fltr > b->cfg_press_thrs)
		{
			b->int_state = BTN_PRESS;
			b->cnt_press = CLIP(b->cnt_press + time_diff, 0, b->cfg_long_press_time);
		}
		if(b->cnt_fltr < b->cfg_release_thrs)
		{
			b->int_state = 0;
			b->cnt_press = 0;
		}
		if(b->cnt_press >= b->cfg_long_press_time) b->int_state = BTN_PRESS_LONG;
	}
	if(b->int_state_prev != b->int_state)
	{
		b->int_state_prev = b->int_state;

		if(b->int_state == BTN_PRESS)
			b->state = BTN_PRESS_SHOT;
		else if(b->int_state == BTN_PRESS_LONG)
			b->state = BTN_PRESS_LONG_SHOT;
		else if(b->int_state == 0)
			b->state = BTN_UNPRESS_SHOT;
	}
	else
	{
		b->state = b->int_state;
	}
}
