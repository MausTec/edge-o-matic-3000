#ifndef __orgasm_control_h
#define __orgasm_control_h

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "vibration_mode_controller.h"
#include <stddef.h>
#include <stdint.h>

// This enum has an associated strings array in orgasm_control.c
typedef enum orgasm_output_mode {
    OC_MANUAL_CONTROL,
    OC_AUTOMAITC_CONTROL,
    _OC_MODE_MAX,
    _OC_MODE_ERROR = -1
} orgasm_output_mode_t;

typedef enum oc_bool {
    ocFALSE,
    ocTRUE,
} oc_bool_t;

void orgasm_control_init(void);
void orgasm_control_tick(void);

// Fetch Data
uint16_t orgasm_control_get_arousal(void);
float orgasm_control_get_arousal_percent(void);
uint8_t orgasm_control_get_motor_speed(void);
float orgasm_control_get_motor_speed_percent(void);
uint16_t orgasm_control_get_last_pressure(void);
uint16_t orgasm_control_get_average_pressure(void);
oc_bool_t orgasm_control_updated(void);
void orgasm_control_clear_update_flag(void);
void orgasm_control_increment_arousal_threshold(int threshold);
void orgasm_control_set_arousal_threshold(int threshold);
int orgasm_control_get_arousal_threshold(void);

// Set Controls
void orgasm_control_control_motor(orgasm_output_mode_t control);

void orgasm_control_pause_control(void);
void orgasm_control_resume_control(void);

orgasm_output_mode_t orgasm_control_get_output_mode(void);
void orgasm_control_set_output_mode(orgasm_output_mode_t mode);
const char* orgasm_control_get_output_mode_str(void);
orgasm_output_mode_t orgasm_control_str_to_output_mode(const char* str);

// Recording Control
void orgasm_control_start_recording(void);
void orgasm_control_stop_recording(void);
oc_bool_t orgasm_control_is_recording(void);

#ifdef __cplusplus
}
#endif

#endif
