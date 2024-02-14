#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct
{
    int16_t debounce_config;
    int16_t debounce_timer;
    bool pressed;
    bool pressed_shot;
    bool unpressed_shot;
} debounce_ctrl_t;

/**
 * @brief Debounce structure initializer
 * 
 * @param ctrl 
 * @param timeout 
 */
inline void debounce_init(debounce_ctrl_t *ctrl, int16_t timeout)
{
    memset((void *)ctrl, 0, sizeof(debounce_ctrl_t));
    ctrl->debounce_config = timeout;
}

/**
 * @brief Callback for debounce engine
 * 
 * @param ctrl 
 * @param time_cb_diff
 * @return true state changed
 * @return false 
 */
inline bool debounce_cb(debounce_ctrl_t *ctrl, bool state_now, int16_t time_cb_diff)
{
    if(ctrl->debounce_timer != 0)
    {
        if(ctrl->debounce_timer < time_cb_diff)
            ctrl->debounce_timer = 0;
        else
            ctrl->debounce_timer -= time_cb_diff;
    }

    ctrl->pressed_shot = false;
    ctrl->unpressed_shot = false;

    if((ctrl->pressed != state_now) &&
       (ctrl->debounce_timer == 0))
    {
        ctrl->debounce_timer = ctrl->debounce_config;
        ctrl->pressed = state_now;
        if(ctrl->pressed) ctrl->pressed_shot = true;
        else ctrl->unpressed_shot = true;
        return true;
    }

    return false;
}

#endif // DEBOUNCE_H
