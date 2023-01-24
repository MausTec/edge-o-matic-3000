#ifndef __orgasm_control_h
#define __orgasm_control_h

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "vibration_mode_controller.h"
#include <stddef.h>
#include <stdint.h>

typedef enum orgasm_output_mode {
    OC_MANUAL_CONTROL,
    OC_AUTOMAITC_CONTROL,
    OC_ORGASM_MODE,
    OC_LOCKOUT_POST_MODE,
} orgasm_output_mode_t;

typedef enum oc_bool {
    ocFALSE,
    ocTRUE,
} oc_bool_t;

void orgasm_control_init(void);
void orgasm_control_tick(void);

// Fetch Data
uint16_t orgasm_control_getArousal(void);
float orgasm_control_getArousalPercent(void);
uint8_t orgasm_control_getMotorSpeed(void);
float orgasm_control_getMotorSpeedPercent(void);
uint16_t orgasm_control_getLastPressure(void);
uint16_t orgasm_control_getAveragePressure(void);
oc_bool_t orgasm_control_updated(void);
int orgasm_control_getDenialCount(void);

// Set Controls
void orgasm_control_controlMotor(orgasm_output_mode_t control);
void orgasm_control_pauseControl(void);
void orgasm_control_resumeControl(void);
orgasm_output_mode_t orgasm_control_get_output_mode(void);

// Recording Control
void orgasm_control_startRecording(void);
void orgasm_control_stopRecording(void);
oc_bool_t orgasm_control_isRecording(void);

// Twitch Detect (In wrong place for 60hz)
void orgasm_control_twitchDetect(void);

// Post orgasm
oc_bool_t orgasm_control_isMenuLocked(void);
oc_bool_t orgasm_control_isPermitOrgasmReached(void);
oc_bool_t orgasm_control_isPostOrgasmReached(void);
void orgasm_control_permitOrgasmNow(int seconds);
void orgasm_control_lockMenuNow(oc_bool_t value);

#ifdef __cplusplus
}
#endif

#endif
