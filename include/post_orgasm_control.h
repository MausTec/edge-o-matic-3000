/**
 * @file post_orgasm_control.h
 * @brief Routines specific to Post-Orgasm and Overstimulation modes.
 * @date 2024-03-27
 *
 * All code related to post-orgasm control should be migrated to this file to reduce the maintenance
 * burden of orgasm_control. Additionally, any references to "torture" mode should be renamed to
 * "overstimulation" mode.
 */

#ifndef __post_orgasm_control_h
#define __post_orgasm_control_h

#ifdef __cplusplus
extern "C" {
#endif

#include "system/event_manager.h"
#include "esp_timer.h"
#include "eom-hal.h"
#include "config.h"
#include "vibration_mode_controller.h"

// Post orgasm Modes
typedef enum post_orgasm_mode {
    Default,
    Milk_o_matic,
    Ruin_orgasm,
    Random_mode
} post_orgasm_mode_t;

void post_orgasm_init(void);
post_orgasm_mode_t post_orgasm_mode_random_select(post_orgasm_mode_t);

#ifdef __cplusplus
}
#endif

#endif
