#include "orgasm_control.h"
#include "accessory_driver.h"
#include "bluetooth_driver.h"
#include "config.h"
#include "data_logger.h"
#include "eom-hal.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "orgasm_detection.h"
#include "system/event_manager.h"
#include "system/websocket_handler.h"
#include "ui/ui.h"
#include "util/i18n.h"
#include "util/running_average.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static const char* TAG = "orgasm_control";

static const char* orgasm_output_mode_str[] = {
    "MANUAL_CONTROL",
    "AUTOMAITC_CONTROL",
};

static struct {
    unsigned long last_update_ms;
    running_average_t* average;
    uint16_t last_value;
    uint16_t pressure_value;
    uint16_t peak_start;
    uint16_t arousal;
    uint8_t update_flag;
} arousal_state;

typedef enum {
    OC_STATE_RUNNING,
    OC_STATE_COOLDOWN,
    OC_STATE_RESUMING,
} oc_control_state_t;

static struct {
    orgasm_output_mode_t output_mode;
    vibration_mode_t vibration_mode;
    uint8_t control_motor;
    uint8_t prev_control_motor;
    float motor_speed;
    oc_control_state_t control_state;
    unsigned long cooldown_start_ms;
    unsigned long resume_start_ms;
    unsigned long cooldown_jitter_ms;
} output_state;

#define update_check(variable, value)                                                              \
    {                                                                                              \
        if (variable != (value)) {                                                                 \
            ESP_LOGD(TAG, "updated: %s = %s", #variable, #value);                                  \
            variable = value;                                                                      \
            arousal_state.update_flag = ocTRUE;                                                    \
        }                                                                                          \
    }

/**
 * @brief Simplified method to set speed, which also handles broadcasting the event.
 * @param speed
 */
static void _set_speed(uint8_t speed) {
    static uint8_t last_speed = 0;
    if (speed == last_speed) return;
    last_speed = speed;

    eom_hal_set_motor_speed(speed);
    event_manager_dispatch(EVT_SPEED_CHANGE, NULL, speed);
    bluetooth_driver_broadcast_speed(speed);
}

void orgasm_control_init(void) {
    output_state.output_mode = OC_MANUAL_CONTROL;
    output_state.vibration_mode = Config.vibration_mode;
    output_state.control_state = OC_STATE_RESUMING;
    output_state.resume_start_ms = esp_timer_get_time() / 1000UL;
    output_state.cooldown_start_ms = 0;
    output_state.cooldown_jitter_ms = 0;

    running_average_init(&arousal_state.average, Config.pressure_smoothing);

    orgasm_detection_init();
}

// Rename to get_vibration_mode_controller();
static const vibration_mode_controller_t* orgasm_control_getVibrationMode() {
    switch (Config.vibration_mode) {
    case Enhancement: return &EnhancementController;

    default:
    case Depletion: return &DepletionController;

    case Pattern: return &PatternController;

    case RampStop: return &RampStopController;
    }
}

/**
 * Main orgasm detection / edging algorithm happens here.
 * This happens with a default update frequency of 50Hz.
 */
static void orgasm_control_updateArousal() {
    // Decay stale arousal value (tick-rate-independent):
    // arousal_decay_rate is the percentage retained per second (0-100).
    // Convert to per-tick factor: decay_factor = (rate/100) ^ (1/hz)
    //   = exp(ln(rate/100) / hz)
    float decay_rate = Config.arousal_decay_rate;
    if (decay_rate < 1) decay_rate = 1;
    if (decay_rate > 99) decay_rate = 99;
    float decay_factor = expf(logf(decay_rate / 100.0f) / (float)Config.update_frequency_hz);
    update_check(arousal_state.arousal, (uint16_t)(arousal_state.arousal * decay_factor));

    // Acquire new pressure and take average:
    arousal_state.pressure_value = eom_hal_get_pressure_reading();
    running_average_add_value(arousal_state.average, arousal_state.pressure_value);
    long p_avg = running_avergae_get_average(arousal_state.average);
    long p_check = Config.use_average_values ? p_avg : arousal_state.pressure_value;

    // Increment arousal
    if (p_check < arousal_state.last_value) {                      // falling edge of peak
        if (arousal_state.last_value > arousal_state.peak_start) { // first tick past peak?
            if (arousal_state.last_value - arousal_state.peak_start >=
                Config.sensitivity_threshold / 10) { // big peak

                update_check(
                    arousal_state.arousal,
                    arousal_state.arousal + (arousal_state.last_value - arousal_state.peak_start)
                );

                arousal_state.peak_start = p_check;
            }
        }

        if (p_check < arousal_state.peak_start) {
            // run this value down to a new minimum after a peak detected.
            arousal_state.peak_start = p_check;
        }
    }

    arousal_state.last_value = p_check;

    // Update accessories:
    if (arousal_state.update_flag) {
        event_manager_dispatch(EVT_AROUSAL_CHANGE, NULL, arousal_state.arousal);
        bluetooth_driver_broadcast_arousal(arousal_state.arousal);
        // websocket_driver_broadcast_arousal(arousal_state.arousal);

        // Update LED for Arousal Color
        if (output_state.output_mode == OC_AUTOMAITC_CONTROL) {
            float arousal_perc = orgasm_control_get_arousal_percent() * 255.0f;
            if (arousal_perc > 255.0f) arousal_perc = 255.0f;
            eom_hal_set_encoder_rgb(arousal_perc, 255 - arousal_perc, 0);
        }
    }
}

static void orgasm_control_updateMotorSpeed() {
    if (!output_state.control_motor) return;

    const vibration_mode_controller_t* controller = orgasm_control_getVibrationMode();
    controller->tick(output_state.motor_speed, arousal_state.arousal);

    unsigned long now_ms = esp_timer_get_time() / 1000UL;

    switch (output_state.control_state) {
    case OC_STATE_RUNNING: {
        // Motor is on, arousal detection active.
        float next_speed = controller->increment();
        update_check(output_state.motor_speed, next_speed);

        if (output_state.motor_speed > Config.motor_max_speed) {
            output_state.motor_speed = Config.motor_max_speed;
        }

        // Detect arousal threshold breach (guard: hold-off must have elapsed)
        if (arousal_state.arousal > Config.sensitivity_threshold && output_state.motor_speed > 0 &&
            (now_ms - output_state.resume_start_ms) >= (unsigned long)Config.arousal_holdoff_ms) {
            float edge_response = controller->on_edge();

            if (edge_response <= 0) {
                // Denial: transition RUNNING -> COOLDOWN
                output_state.control_state = OC_STATE_COOLDOWN;
                output_state.cooldown_start_ms = now_ms;
                output_state.cooldown_jitter_ms =
                    (Config.cooldown_random_ms > 0)
                        ? (unsigned long)(random() % Config.cooldown_random_ms)
                        : 0;
                controller->stop();
                output_state.motor_speed = 0;
                arousal_state.update_flag = ocTRUE;
                event_manager_dispatch(EVT_ORGASM_DENIAL, NULL, 0);
                ESP_LOGD(
                    TAG, "RUNNING -> COOLDOWN (jitter: %lu ms)", output_state.cooldown_jitter_ms
                );
            } else {
                // Permit: stay running at the returned speed
                output_state.motor_speed = edge_response;
                arousal_state.update_flag = ocTRUE;
                event_manager_dispatch(EVT_ORGASM_PERMIT, NULL, 0);
            }
        }
        break;
    }

    case OC_STATE_COOLDOWN: {
        // Motor is off, waiting for cooldown to expire.
        output_state.motor_speed = 0;

        // Anti-twitch: reset cooldown timer on arousal spike
        if (arousal_state.arousal > Config.sensitivity_threshold) {
            output_state.cooldown_start_ms = now_ms;
        }

        // Check if cooldown period has elapsed
        unsigned long cooldown_total =
            (unsigned long)Config.cooldown_delay_ms + output_state.cooldown_jitter_ms;
        if ((now_ms - output_state.cooldown_start_ms) >= cooldown_total) {
            // Transition COOLDOWN -> RESUMING
            output_state.control_state = OC_STATE_RESUMING;
            output_state.resume_start_ms = now_ms;
            output_state.motor_speed = controller->start();
            ESP_LOGD(TAG, "COOLDOWN -> RESUMING");
        }
        break;
    }

    case OC_STATE_RESUMING: {
        // Motor restarting, hold-off timer active.
        float next_speed = controller->increment();
        update_check(output_state.motor_speed, next_speed);

        if (output_state.motor_speed > Config.motor_max_speed) {
            output_state.motor_speed = Config.motor_max_speed;
        }

        // Check if hold-off period has elapsed
        if ((now_ms - output_state.resume_start_ms) >= (unsigned long)Config.arousal_holdoff_ms) {
            output_state.control_state = OC_STATE_RUNNING;
            ESP_LOGD(TAG, "RESUMING -> RUNNING");
        }
        break;
    }

    default:
        // recover from corrupted state
        output_state.control_state = OC_STATE_RESUMING;
        output_state.resume_start_ms = now_ms;
        break;
    }

    // Control motor if we are not manually doing so.
    if (output_state.control_motor) {
        uint8_t speed = orgasm_control_get_motor_speed();
        _set_speed(speed);
    }
}

orgasm_output_mode_t orgasm_control_get_output_mode(void) {
    return output_state.output_mode;
}

const char* orgasm_control_get_output_mode_str(void) {
    if (output_state.output_mode < _OC_MODE_MAX) {
        return orgasm_output_mode_str[output_state.output_mode];
    } else {
        return "";
    }
}

orgasm_output_mode_t orgasm_control_str_to_output_mode(const char* str) {
    for (int i = 0; i < _OC_MODE_MAX; i++) {
        if (!strcasecmp(str, orgasm_output_mode_str[i])) {
            return (orgasm_output_mode_t)i;
        }
    }

    return -1;
}

/**
 * @deprecated Recording functions now delegate to data_logger module.
 * Use data_logger_start_recording(), data_logger_stop_recording(),
 * and data_logger_is_recording() directly.
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

void orgasm_control_start_recording() {
    data_logger_start_recording();
}

void orgasm_control_stop_recording() {
    data_logger_stop_recording();
}

oc_bool_t orgasm_control_is_recording() {
    return (oc_bool_t)data_logger_is_recording();
}

#pragma GCC diagnostic pop

void orgasm_control_tick() {
    if (Config.update_frequency_hz == 0) return;

    unsigned long millis = esp_timer_get_time() / 1000UL;
    unsigned long update_frequency_ms = 1000UL / Config.update_frequency_hz;

    if (millis - arousal_state.last_update_ms > update_frequency_ms) {
        orgasm_control_updateArousal();
        orgasm_control_updateMotorSpeed();

        // Orgasm detection — runs after arousal and motor updates
        long p_check_for_det = Config.use_average_values ? orgasm_control_get_average_pressure()
                                                         : arousal_state.pressure_value;
        orgasm_detection_tick((uint16_t)p_check_for_det, arousal_state.arousal);

        // Clench arousal boost: add to arousal while in sustained/rhythmic phase
        if (Config.od_clench_arousal_boost) {
            orgasm_detect_state_t od_state = orgasm_detection_get_state();
            if (od_state == OD_STATE_SUSTAINED || od_state == OD_STATE_RHYTHMIC) {
                arousal_state.arousal += Config.od_clench_arousal_boost_amount;
                arousal_state.update_flag = ocTRUE;
            }
        }

        arousal_state.last_update_ms = millis;

        // Data logging (CSV + serial) handled by data_logger module
        data_logger_tick(arousal_state.last_update_ms);
    }
}

oc_bool_t orgasm_control_updated() {
    return arousal_state.update_flag;
}

void orgasm_control_clear_update_flag(void) {
    arousal_state.update_flag = ocFALSE;
}

/**
 * Returns a normalized motor speed from 0..255
 * @return normalized motor speed byte
 */
uint8_t orgasm_control_get_motor_speed() {
    if (output_state.motor_speed < 0) return 0;
    if (output_state.motor_speed > 255)
        return 255;
    else
        return (uint8_t)floor(output_state.motor_speed);
}

float orgasm_control_get_motor_speed_percent() {
    return (float)orgasm_control_get_motor_speed() / 255.0f;
}

uint16_t orgasm_control_get_arousal() {
    return arousal_state.arousal;
}

float orgasm_control_get_arousal_percent() {
    if (Config.sensitivity_threshold == 0) return 1.0;
    return (float)arousal_state.arousal / Config.sensitivity_threshold;
}

void orgasm_control_increment_arousal_threshold(int threshold) {
    orgasm_control_set_arousal_threshold(Config.sensitivity_threshold + threshold);
}

void orgasm_control_set_arousal_threshold(int threshold) {
    // Sensitivity threshold of 0 prevents horrible issues from happening.
    // It also prevents confusing the customers, which is a big win I'd say.
    Config.sensitivity_threshold = threshold >= 10 ? threshold : 10;
    config_enqueue_save(300);
}

int orgasm_control_get_arousal_threshold(void) {
    return Config.sensitivity_threshold;
}

uint16_t orgasm_control_get_last_pressure() {
    return arousal_state.pressure_value;
}

uint16_t orgasm_control_get_average_pressure() {
    return running_avergae_get_average(arousal_state.average);
}

void orgasm_control_control_motor(orgasm_output_mode_t control) {
    orgasm_control_set_output_mode(control);
}

void orgasm_control_set_output_mode(orgasm_output_mode_t mode) {
    orgasm_output_mode_t old = output_state.output_mode;
    output_state.output_mode = mode;
    output_state.control_motor = mode != OC_MANUAL_CONTROL;

    if (old == OC_MANUAL_CONTROL) {
        const vibration_mode_controller_t* controller = orgasm_control_getVibrationMode();
        output_state.motor_speed = controller->start();
        output_state.control_state = OC_STATE_RESUMING;
        output_state.resume_start_ms = esp_timer_get_time() / 1000UL;
    } else if (mode == OC_MANUAL_CONTROL) {
        const vibration_mode_controller_t* controller = orgasm_control_getVibrationMode();
        output_state.motor_speed = controller->stop();
    }

    // TODO: This is handled by the UI rendering process, I think??
    eom_hal_set_encoder_rgb(
        (mode + 1) & 0x04 ? 0xFF : 0x00,
        (mode + 1) & 0x02 ? 0xFF : 0x00,
        (mode + 1) & 0x01 ? 0xFF : 0x00
    );

    event_manager_dispatch(EVT_MODE_SET, NULL, mode);
}

void orgasm_control_pause_control() {
    output_state.prev_control_motor = output_state.control_motor;
    output_state.control_motor = OC_MANUAL_CONTROL;
}

void orgasm_control_resume_control() {
    output_state.control_motor = output_state.prev_control_motor;
}
