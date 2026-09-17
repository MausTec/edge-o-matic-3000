#ifndef __actions__motor_h
#define __actions__motor_h

#ifdef __cplusplus
extern "C" {
#endif

#include "mt_actions.h"

/**
 * @brief Register motor/output-mode host functions (get_motor_speed,
 * get_output_mode, request_motor_control, release_motor_control,
 * set_motor_speed).
 */
void action_motor_init(void);

#ifdef __cplusplus
}
#endif

#endif
