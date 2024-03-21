#include "orgasm_control.h"
#include "accessory_driver.h"
#include "bluetooth_driver.h"
#include "config.h"
#include "eom-hal.h"
#include "esp_log.h"
#include "esp_timer.h"
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
    "ORGASM_MODE",
};

static struct {
    unsigned long last_update_ms;
    running_average_t* average;
    uint16_t last_value;
    uint16_t pressure_value;
    uint16_t peak_start;
    uint16_t arousal;
    uint8_t update_flag;
    uint8_t denial_count;
} arousal_state;

static struct {
    orgasm_output_mode_t output_mode;
    vibration_mode_t vibration_mode;
    unsigned long motor_stop_time;
    unsigned long motor_start_time;
    unsigned long edge_time_out; // 10000?
    unsigned long random_additional_delay;
    int twitch_count;
    uint8_t control_motor;
    uint8_t prev_control_motor;
    float motor_speed;
} output_state;

static struct {
    // File Writer
    unsigned long recording_start_ms;
    FILE* logfile;
} logger_state;

static struct {
    //  Post Orgasm Clench variables
    long clench_pressure_threshold; //  4096?
    int clench_duration;

    // Autoedging Time and Post-Orgasm varables
    unsigned long auto_edging_start_millis;
    unsigned long post_orgasm_start_millis;
    unsigned long post_orgasm_duration_millis;
    unsigned long clench_start_millis;
    long clench_duration_millis;
    oc_bool_t menu_is_locked;
    oc_bool_t detected_orgasm;
    int post_orgasm_duration_seconds;
} post_orgasm_state;

#define update_check(variable, value)                                                              \
    {                                                                                              \
        if (variable != (value)) {                                                                 \
            ESP_LOGD(TAG, "updated: %s = %s", #variable, #value);                                  \
            variable = value;                                                                      \
            arousal_state.update_flag = ocTRUE;                                                    \
        }                                                                                          \
    }

void orgasm_control_init(void) {
    output_state.output_mode = OC_MANUAL_CONTROL;
    output_state.vibration_mode = Config.vibration_mode;
    output_state.edge_time_out = 10000;
    post_orgasm_state.clench_pressure_threshold = 4096;

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
    update_check(arousal_state.pressure_value, eom_hal_get_pressure_reading());
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

    // detect muscle clenching.  Used in Edging+orgasm routine to detect an orgasm
    // Can also be used as an other method to compliment detecting edging

    // raise clench threshold to pressure - 1/2 sensitivity
    long current_time = (esp_timer_get_time() / 1000UL);
    if (p_check >=
        (post_orgasm_state.clench_pressure_threshold + Config.clench_pressure_sensitivity)) {
        post_orgasm_state.clench_pressure_threshold =
            (p_check - (Config.clench_pressure_sensitivity / 2));
    }

    // Start counting clench time if pressure over threshold
    if (p_check >= post_orgasm_state.clench_pressure_threshold) {
        post_orgasm_state.clench_duration_millis =
            current_time - post_orgasm_state.clench_start_millis;

        // Orgasm detected
        if (post_orgasm_state.clench_duration_millis >= Config.clench_time_to_orgasm_ms &&
            orgasm_control_isPermitOrgasmReached()) {
            post_orgasm_state.detected_orgasm = ocTRUE;
            post_orgasm_state.clench_duration = 0;
        }

        // ajust arousal if Clench_detector in Edge is turned on
        if (Config.clench_detector_in_edging) {
            if (post_orgasm_state.clench_duration_millis > Config.clench_time_threshold_ms) {
                arousal_state.arousal += 5;
                arousal_state.update_flag = ocTRUE;
            }
        }

        // desensitize clench threshold when clench too long. this is to stop arousal from going up
        if (post_orgasm_state.clench_duration_millis >= Config.max_clench_duration_ms &&
            !orgasm_control_isPermitOrgasmReached(
            )) { // Allow higher clench duration when orgasm permitted
            post_orgasm_state.clench_pressure_threshold += 10;
            //            post_orgasm_state.clench_pressure_threshold += 100000; // desensitize
            //            enough to reduce clench cheating
            post_orgasm_state.clench_duration_millis = Config.max_clench_duration_ms; // ms
        }

        // when not clenching lower clench time and decay clench threshold
    } else {
        if (output_state.motor_speed > 0 ||
            output_state.output_mode ==
                OC_MANUAL_CONTROL) { // decay only in Manual mode or if not in a denial period
            post_orgasm_state.clench_start_millis = current_time;
            post_orgasm_state.clench_duration_millis -= 150; // ms

            if (post_orgasm_state.clench_duration_millis <= 0) {
                post_orgasm_state.clench_duration_millis = 0;
                // clench pressure threshold value decays over time to a min of pressure + 1/2
                // sensitivity
                if ((p_check + (Config.clench_pressure_sensitivity / 2)) <
                    post_orgasm_state.clench_pressure_threshold) {
                    post_orgasm_state.clench_pressure_threshold *= 0.99;
                }
            }
        }
    } // END of clench detector

    // Update accessories:
    if (arousal_state.update_flag) {
        accessory_driver_broadcast_arousal(arousal_state.arousal);
        bluetooth_driver_broadcast_arousal(arousal_state.arousal);
        // websocket_driver_broadcast_arousal(arousal_state.arousal);

        // Update LED for Arousal Color
        if (output_state.output_mode == OC_AUTOMAITC_CONTROL) {
            float arousal_perc = orgasm_control_getArousalPercent() * 255.0f;
            if (arousal_perc > 255.0f) arousal_perc = 255.0f;
            eom_hal_set_encoder_rgb(arousal_perc, 255 - arousal_perc, 0);
        }
    }
}

static void orgasm_control_updateMotorSpeed() {
    if (!output_state.control_motor) return;

    const vibration_mode_controller_t* controller = orgasm_control_getVibrationMode();
    controller->tick(output_state.motor_speed, arousal_state.arousal);

    // Calculate timeout delay
    oc_bool_t time_out_over = ocFALSE;
    long on_time = (esp_timer_get_time() / 1000UL) - output_state.motor_start_time;

    if ((esp_timer_get_time() / 1000UL) - output_state.motor_stop_time >
        Config.edge_delay + output_state.random_additional_delay) {
        time_out_over = ocTRUE;
    }

    // Ope, orgasm incoming! Stop it!
    if (!time_out_over) {
        orgasm_control_twitchDetect();

    } else if (arousal_state.arousal > Config.sensitivity_threshold &&
               output_state.motor_speed > 0 && on_time > Config.minimum_on_time) {
        // The motor_speed check above, btw, is so we only hit this once per peak.
        // Set the motor speed to 0, set stop time, and determine the new additional random time.
        output_state.motor_speed = controller->stop();
        output_state.motor_stop_time = (esp_timer_get_time() / 1000UL);
        output_state.motor_start_time = 0;
        arousal_state.denial_count++;
        arousal_state.update_flag = ocTRUE;

        // If Max Additional Delay is not disabled, caculate a new delay every time the motor is
        // stopped.
        if (Config.max_additional_delay != 0) {
            output_state.random_additional_delay = random() % Config.max_additional_delay;
        }

        // Start from 0
    } else if (output_state.motor_speed == 0 && output_state.motor_start_time == 0) {
        output_state.motor_speed = controller->start();
        output_state.motor_start_time = (esp_timer_get_time() / 1000UL);
        output_state.random_additional_delay = 0;
        arousal_state.update_flag = ocTRUE;

        // Increment or Change
    } else {
        update_check(output_state.motor_speed, controller->increment());
    }

    // Control motor if we are not manually doing so.
    if (output_state.control_motor) {
        uint8_t speed = orgasm_control_getMotorSpeed();
        eom_hal_set_motor_speed(speed);
        accessory_driver_broadcast_speed(speed);
        bluetooth_driver_broadcast_speed(speed);
    }
}

static void orgasm_control_updateEdgingTime() { // Edging+Orgasm timer
    // Make sure menu_is_locked is turned off in Manual mode
    if (output_state.output_mode == OC_MANUAL_CONTROL) {
        post_orgasm_state.menu_is_locked = ocFALSE;
        post_orgasm_state.post_orgasm_duration_seconds = Config.post_orgasm_duration_seconds;
        return;
    }

    // keep edging start time to current time as long as system is not in Edge-Orgasm mode 2
    if (output_state.output_mode != OC_ORGASM_MODE) {
        post_orgasm_state.auto_edging_start_millis = (esp_timer_get_time() / 1000UL);
        post_orgasm_state.post_orgasm_start_millis = 0;
    }

    // Lock Menu if turned on. and in Edging_orgasm mode
    if (Config.edge_menu_lock && !post_orgasm_state.menu_is_locked) {
        // Lock only after 2 minutes
        if ((esp_timer_get_time() / 1000UL) >
            post_orgasm_state.auto_edging_start_millis + (2 * 60 * 1000)) {
            post_orgasm_state.menu_is_locked = ocTRUE;
            arousal_state.update_flag = ocTRUE;
        }
    }

    // Pre-Orgasm loop -- Orgasm is permited
    if (orgasm_control_isPermitOrgasmReached() && !orgasm_control_isPostOrgasmReached()) {
        if (output_state.control_motor) {
            orgasm_control_pauseControl(); // make sure orgasm is now possible
        }

        // now detect the orgasm to start post orgasm torture timer
        if (post_orgasm_state.detected_orgasm) {
            post_orgasm_state.post_orgasm_start_millis =
                (esp_timer_get_time() / 1000UL); // Start Post orgasm torture timer
            // Lock menu if turned on
            if (Config.post_orgasm_menu_lock && !post_orgasm_state.menu_is_locked) {
                post_orgasm_state.menu_is_locked = ocTRUE;
            }

            eom_hal_set_encoder_rgb(255, 0, 0);
        } else {
            eom_hal_set_encoder_rgb(0, 255, 0);
        }

        // raise motor speed to max speep. protect not to go higher than max
        if (output_state.motor_speed <= (Config.motor_max_speed - 5)) {
            output_state.motor_speed += 5;
        } else {
            update_check(output_state.motor_speed, Config.motor_max_speed);
        }
    }

    // Post Orgasm loop
    if (orgasm_control_isPostOrgasmReached()) {
        post_orgasm_state.post_orgasm_duration_millis =
            (post_orgasm_state.post_orgasm_duration_seconds * 1000);

        // Detect if within post orgasm session
        if ((esp_timer_get_time() / 1000UL) < (post_orgasm_state.post_orgasm_start_millis +
                                               post_orgasm_state.post_orgasm_duration_millis)) {
            output_state.motor_speed = Config.motor_max_speed;
        } else {                                  // Post_orgasm timer reached
            if (output_state.motor_speed > 0) { // Ramp down motor speed to 0
                output_state.motor_speed = output_state.motor_speed - 1;
            } else {
                post_orgasm_state.menu_is_locked = ocFALSE;
                post_orgasm_state.detected_orgasm = ocFALSE;
                output_state.motor_speed = 0;
                eom_hal_set_motor_speed(output_state.motor_speed);
                accessory_driver_broadcast_speed(output_state.motor_speed);
                bluetooth_driver_broadcast_speed(output_state.motor_speed);
                orgasm_control_set_output_mode(OC_MANUAL_CONTROL);
            }
        }
    }
    // Control output while motor control is paused
    if (output_state.control_motor == OC_MANUAL_CONTROL) {
        uint8_t speed = orgasm_control_getMotorSpeed();
        eom_hal_set_motor_speed(speed);
        accessory_driver_broadcast_speed(speed);
        bluetooth_driver_broadcast_speed(speed);
    }
}

void orgasm_control_twitchDetect() {
    if (arousal_state.arousal > Config.sensitivity_threshold) {
        output_state.motor_stop_time = (esp_timer_get_time() / 1000UL);
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
void orgasm_control_startRecording() {
    if (logger_state.logfile) {
        orgasm_control_stopRecording();
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
            "millis,pressure,avg_pressure,arousal,motor_speed,sensitivity_threshold,"
            "clench_pressure_threshold,clench_duration"
        );

        ui_set_icon(UI_ICON_RECORD, RECORD_ICON_RECORDING);
        char* fntok = basename(logfile_name);
        ui_toast(_("Recording started:\n%s"), fntok);
    }

    free(logfile_name);
}

void orgasm_control_stopRecording() {
    if (logger_state.logfile != NULL) {
        ui_toast_blocking("%s", _("Stopping..."));
        ESP_LOGI(TAG, "Closing logfile.");
        fclose(logger_state.logfile);
        logger_state.logfile = NULL;
        ui_set_icon(UI_ICON_RECORD, -1);
        ui_toast("%s", _("Recording stopped."));
    }
}

oc_bool_t orgasm_control_isRecording() {
    return (oc_bool_t) !!logger_state.logfile;
}

void orgasm_control_tick() {
    if (Config.update_frequency_hz == 0) return;

    unsigned long millis = esp_timer_get_time() / 1000UL;
    unsigned long update_frequency_ms = 1000UL / Config.update_frequency_hz;

    if (millis - arousal_state.last_update_ms > update_frequency_ms) {
        orgasm_control_updateArousal();
        orgasm_control_updateEdgingTime();
        orgasm_control_updateMotorSpeed();
        arousal_state.last_update_ms = millis;

        // Data for logfile or classic log.
        char data_csv[255];
        snprintf(
            data_csv,
            255,
            "%d,%d,%d,%d,%ld,%ld",
            orgasm_control_getAveragePressure(),
            orgasm_control_getArousal(),
            eom_hal_get_motor_speed(),
            Config.sensitivity_threshold,
            post_orgasm_state.clench_pressure_threshold,
            post_orgasm_state.clench_duration_millis
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

int orgasm_control_getDenialCount() {
    return arousal_state.denial_count;
}

/**
 * Returns a normalized motor speed from 0..255
 * @return normalized motor speed byte
 */
uint8_t orgasm_control_getMotorSpeed() {
    if (output_state.motor_speed < 0) return 0;
    if (output_state.motor_speed > 255)
        return 255;
    else
        return (uint8_t)floor(output_state.motor_speed);
}

float orgasm_control_getMotorSpeedPercent() {
    return (float)orgasm_control_getMotorSpeed() / 255.0f;
}

uint16_t orgasm_control_getArousal() {
    return arousal_state.arousal;
}

float orgasm_control_getArousalPercent() {
    if (Config.sensitivity_threshold == 0) return 1.0;
    return (float)arousal_state.arousal / Config.sensitivity_threshold;
}

void orgasm_control_increment_arousal_threshold(int threshold) {
    orgasm_control_set_arousal_threshold(Config.sensitivity_threshold + threshold);
}

void orgasm_control_set_arousal_threshold(int threshold) {
    Config.sensitivity_threshold = threshold >= 0 ? threshold : 0;
    config_enqueue_save(300);
}

int orgasm_control_get_arousal_threshold(void) {
    return Config.sensitivity_threshold;
}

uint16_t orgasm_control_getLastPressure() {
    return arousal_state.pressure_value;
}

uint16_t orgasm_control_getAveragePressure() {
    return running_avergae_get_average(arousal_state.average);
}

void orgasm_control_controlMotor(orgasm_output_mode_t control) {
    orgasm_control_set_output_mode(control);
}

void orgasm_control_set_output_mode(orgasm_output_mode_t control) {
    orgasm_output_mode_t old = output_state.output_mode;
    output_state.output_mode = control;
    output_state.control_motor = control != OC_MANUAL_CONTROL;

    if (old == OC_MANUAL_CONTROL) {
        const vibration_mode_controller_t* controller = orgasm_control_getVibrationMode();
        output_state.motor_speed = controller->start();
    } else if (control == OC_MANUAL_CONTROL) {
        const vibration_mode_controller_t* controller = orgasm_control_getVibrationMode();
        output_state.motor_speed = controller->stop();
    }

    eom_hal_set_encoder_rgb(
        (control + 1) & 0x04 ? 0xFF : 0x00,
        (control + 1) & 0x02 ? 0xFF : 0x00,
        (control + 1) & 0x01 ? 0xFF : 0x00
    );
}

void orgasm_control_pauseControl() {
    output_state.prev_control_motor = output_state.control_motor;
    output_state.control_motor = OC_MANUAL_CONTROL;
}

void orgasm_control_resumeControl() {
    output_state.control_motor = output_state.prev_control_motor;
}

void orgasm_control_permitOrgasmNow(int seconds) {
    post_orgasm_state.detected_orgasm = ocFALSE;
    orgasm_control_set_output_mode(OC_ORGASM_MODE);
    post_orgasm_state.auto_edging_start_millis =
        (esp_timer_get_time() / 1000UL) - (Config.auto_edging_duration_minutes * 60 * 1000);
    post_orgasm_state.post_orgasm_duration_seconds = seconds;
}

oc_bool_t orgasm_control_isPermitOrgasmReached() {
    // Detect if edging time has passed
    if ((esp_timer_get_time() / 1000UL) > (post_orgasm_state.auto_edging_start_millis +
                                           (Config.auto_edging_duration_minutes * 60 * 1000))) {
        return ocTRUE;
    } else {
        return ocFALSE;
    }
}

oc_bool_t orgasm_control_isPostOrgasmReached() {
    // Detect if after orgasm
    if (post_orgasm_state.post_orgasm_start_millis > 0) {
        return ocTRUE;
    } else {
        return ocFALSE;
    }
}

oc_bool_t orgasm_control_isMenuLocked() {
    return post_orgasm_state.menu_is_locked;
};

void orgasm_control_lockMenuNow(oc_bool_t value) {
    post_orgasm_state.menu_is_locked = value;
}
