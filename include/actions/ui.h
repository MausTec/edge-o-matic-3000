#ifndef __actions__ui_h
#define __actions__ui_h

#ifdef __cplusplus
extern "C" {
#endif

#include "mt_actions.h"

/**
 * @brief Register UI host functions (set_led_color, notify, lock_controls,
 * unlock_controls).
 */
void action_ui_init(void);

#ifdef __cplusplus
}
#endif

#endif
