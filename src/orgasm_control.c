#include "orgasm_control.h"
#include "accessory_driver.h"
#include "bluetooth_driver.h"
#include "config.h"
#include "eom-hal.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "system/event_manager.h"
#include "system/websocket_handler.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "util/i18n.h"
#include "util/running_average.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

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

static struct {
    orgasm_output_mode_t output_mode;
    vibration_mode_t vibration_mode;
    uint8_t control_motor;
    uint8_t prev_control_motor;
    float motor_speed;
} output_state;

static struct {
    // File Writer
    unsigned long recording_start_ms;
    FILE* logfile;
} logger_state;

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

    running_average_init(&arousal_state.average, Config.pressure_smoothing);
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
    // Decay stale arousal value:
    update_check(arousal_state.arousal, arousal_state.arousal * 0.99);

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

    float motor_increment =
        ((float)Config.motor_max_speed /
         ((float)Config.update_frequency_hz * (float)Config.motor_ramp_time_s));

    // Ope, orgasm incoming! Stop it!
    if (arousal_state.arousal > Config.sensitivity_threshold && output_state.motor_speed > 0) {
        output_state.motor_speed = fmaxf(
            -255.0f,
            -0.5f * (float)Config.motor_ramp_time_s * (float)Config.update_frequency_hz *
                motor_increment
        );
        arousal_state.update_flag = ocTRUE;

        event_manager_dispatch(EVT_ORGASM_DENIAL, NULL, 0);

    } else if (output_state.motor_speed < Config.motor_max_speed) {
        update_check(output_state.motor_speed, output_state.motor_speed + motor_increment);

    } else if (output_state.motor_speed > Config.motor_max_speed) {
        output_state.motor_speed = Config.motor_max_speed;
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
 * \todo Recording functions don't need to be here.
 */
void orgasm_control_start_recording() {
    if (logger_state.logfile) {
        orgasm_control_stop_recording();
    }

    ui_toast_blocking("%s", _("Preapring recording..."));

    time_t now;
    struct tm timeinfo;
    char filename_date[32];
    time(&now);

    if (!localtime_r(&now, &timeinfo)) {
        ESP_LOGE(TAG, "Failed to obtain time");
        sniprintf(filename_date, 32, "%lld", (esp_timer_get_time() / 1000UL));
    } else {
        strftime(filename_date, 32, "%Y%m%d-%H%M%S", &timeinfo);
    }

    char* logfile_name = NULL;
    asiprintf(&logfile_name, "%s/log-%s.csv", eom_hal_get_sd_mount_point(), filename_date);

    if (!logfile_name) {
        ESP_LOGE(TAG, "Logfile filename buffer issues.");
        ui_toast("%s", _("Error opening logfile!"));
    }

    ESP_LOGI(TAG, "Opening logfile: %s", logfile_name);
    logger_state.logfile = fopen(logfile_name, "w+");

    if (!logger_state.logfile) {
        ESP_LOGE(TAG, "Couldn't open logfile to save! (%s)", logfile_name);
        ui_toast("%s", _("Error opening logfile!"));
    } else {
        logger_state.recording_start_ms = (esp_timer_get_time() / 1000UL);

        fprintf(
            logger_state.logfile,
            "millis,pressure,avg_pressure,arousal,motor_speed,sensitivity_threshold"
        );

        ui_set_icon(UI_ICON_RECORD, RECORD_ICON_RECORDING);
        char* fntok = basename(logfile_name);
        ui_toast(_("Recording started:\n%s"), fntok);
    }

    free(logfile_name);
}

void orgasm_control_stop_recording() {
    if (logger_state.logfile != NULL) {
        ui_toast_blocking("%s", _("Stopping..."));
        ESP_LOGI(TAG, "Closing logfile.");
        fclose(logger_state.logfile);
        logger_state.logfile = NULL;
        ui_set_icon(UI_ICON_RECORD, -1);
        ui_toast("%s", _("Recording stopped."));
    }
}

oc_bool_t orgasm_control_is_recording() {
    return (oc_bool_t) !!logger_state.logfile;
}

void orgasm_control_tick() {
    if (Config.update_frequency_hz == 0) return;

    unsigned long millis = esp_timer_get_time() / 1000UL;
    unsigned long update_frequency_ms = 1000UL / Config.update_frequency_hz;

    if (millis - arousal_state.last_update_ms > update_frequency_ms) {
        orgasm_control_updateArousal();
        orgasm_control_updateMotorSpeed();
        arousal_state.last_update_ms = millis;

        // Data for logfile or classic log.
        char data_csv[255];
        snprintf(
            data_csv,
            255,
            "%d,%d,%d,%d",
            orgasm_control_get_average_pressure(),
            orgasm_control_get_arousal(),
            eom_hal_get_motor_speed(),
            Config.sensitivity_threshold
        );

        // Write out to logfile, which includes millis:
        if (logger_state.logfile != NULL) {
            fprintf(
                logger_state.logfile,
                "%ld,%s\n",
                arousal_state.last_update_ms - logger_state.recording_start_ms,
                data_csv
            );
        }

        // Write to console for classic log mode:
        if (Config.classic_serial) {
            printf("%s\n", data_csv);
        }
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
