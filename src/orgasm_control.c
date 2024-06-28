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
#include "orgasm_trigger_control.h"
#include "post_orgasm_control.h"

static const char* TAG = "orgasm_control";

static const char* orgasm_output_mode_str[] = {
    "MANUAL_CONTROL",
    "AUTOMAITC_CONTROL",
    "ORGASM_MODE",
};

session_t session_state;

static struct {
    orgasm_control_state_t state;
    oc_bool_t menu_is_locked;
    unsigned long edging_start_millis;
    uint8_t orgasm_count;
    float motor_speed;
    event_handler_node_t* _h_orgasm_is_permited;
    event_handler_node_t* _h_orgasm_control_shutdown;
    event_handler_node_t* _h_orgasm_control_session_start;
    event_handler_node_t* _h_orgasm_control_session_setup;
    event_handler_node_t* _h_orgasm;
} orgasm_control;

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
    unsigned long motor_stop_time;
    unsigned long motor_start_time;
    unsigned long random_additional_delay;
    int twitch_count;
    uint8_t control_motor;
    uint8_t prev_control_motor;
    float motor_speed;
    float motor_increment;
    event_handler_node_t* _h_motor_speed_change;
} output_state;

static struct {
    // File Writer
    unsigned long recording_start_ms;
    FILE* logfile;
} logger_state;

static struct {
    //  Clench variables
    long clench_pressure_threshold; //  4096?
    long clench_duration_millis;
    unsigned long clench_start_millis;
} clench_state;

volatile static struct {
    uint8_t edge_count;
    event_handler_node_t* _h_edge_count;
} edge_state = { 0 };

static void _evt_orgasm_denial(
    const char* evt, EVENT_HANDLER_ARG_TYPE eap, int eai, EVENT_HANDLER_ARG_TYPE hap
) {
    edge_state.edge_count += 1;
}

static void _evt_orgasm_is_permited(
    const char* evt, EVENT_HANDLER_ARG_TYPE eap, int eai, EVENT_HANDLER_ARG_TYPE hap
) {
    if ( orgasm_control.state == EDGING_ORGASM ) {
        orgasm_control.state = ORGASM_IS_PERMITED;
    }
}

static void _evt_orgasm_start(
    const char* evt, EVENT_HANDLER_ARG_TYPE eap, int orgasm_count, EVENT_HANDLER_ARG_TYPE hap
) {
    if ( orgasm_control.state == ORGASM_IS_PERMITED ) {
        orgasm_control.state = POST_ORGASM;
        orgasm_control.orgasm_count = orgasm_count;
    }
    // if not permited then you got a ruined orgasm 
}

static void _evt_orgasm_control_session_start(
    const char* evt, EVENT_HANDLER_ARG_TYPE eap, int eai, EVENT_HANDLER_ARG_TYPE hap
) {
    orgasm_control.state = SESSION_START;
}

static void _evt_orgasm_control_session_setup(
    const char* evt, EVENT_HANDLER_ARG_TYPE eap, int eai, EVENT_HANDLER_ARG_TYPE hap
) {
    orgasm_control.state = SESSION_SETUP;
}

static void _evt_orgasm_control_shutdown(
    const char* evt, EVENT_HANDLER_ARG_TYPE eap, int eai, EVENT_HANDLER_ARG_TYPE hap
) {
    orgasm_control.state = SHUTDOWN;
}

static void _evt_motor_speed_change(
    const char* evt, EVENT_HANDLER_ARG_TYPE eap, int speed, EVENT_HANDLER_ARG_TYPE hap
) {
    static uint8_t last_speed = 0;
    if (speed == last_speed) return;
    last_speed = speed;

    eom_hal_set_motor_speed(speed);
    bluetooth_driver_broadcast_speed(speed);
    output_state.motor_speed = speed;
}

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
    clench_state.clench_pressure_threshold = 4096;

    running_average_init(&arousal_state.average, Config.pressure_smoothing);
    if (edge_state._h_edge_count == NULL) {
        edge_state._h_edge_count =
            event_manager_register_handler(EVT_ORGASM_DENIAL, &_evt_orgasm_denial, NULL);
    }
    if (orgasm_control._h_orgasm == NULL) {
        orgasm_control._h_orgasm =
            event_manager_register_handler(EVT_ORGASM_START, &_evt_orgasm_start, NULL);
    }
    if (output_state._h_motor_speed_change == NULL) {
        output_state._h_motor_speed_change =
            event_manager_register_handler(EVT_SPEED_CHANGE, &_evt_motor_speed_change, NULL);
    }
    if (orgasm_control._h_orgasm_is_permited == NULL) {
        orgasm_control._h_orgasm_is_permited = 
            event_manager_register_handler(EVT_ORGASM_CONTROL_ORGASM_IS_PERMITED, &_evt_orgasm_is_permited, NULL);
    }
    if (orgasm_control._h_orgasm_control_shutdown == NULL){
        orgasm_control._h_orgasm_control_shutdown =
            event_manager_register_handler(EVT_ORGASM_CONTROL_SHUTDOWN, &_evt_orgasm_control_shutdown, NULL);
    }
    if (orgasm_control._h_orgasm_control_session_start == NULL){
        orgasm_control._h_orgasm_control_session_start =
            event_manager_register_handler(EVT_ORGASM_CONTROL_SESSION_START, &_evt_orgasm_control_session_start, NULL);
    }
    if (orgasm_control._h_orgasm_control_session_setup == NULL){
        orgasm_control._h_orgasm_control_session_setup =
            event_manager_register_handler(EVT_ORGASM_CONTROL_SESSION_SETUP, &_evt_orgasm_control_session_setup, NULL);
    }
    orgasm_trigger_init();
    post_orgasm_init();
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

    long clench_duration;
    clench_duration = orgasm_control_clench_detect(p_check);
    if (Config.clench_detector_in_edging) {
        if (clench_duration > Config.clench_time_threshold_ms &&
                clench_duration < Config.max_clench_duration_ms) {
            arousal_state.arousal += 5;
            arousal_state.update_flag = ocTRUE;
        }
    }

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

    // Calculate timeout delay
    oc_bool_t time_out_over = ocFALSE;
    long on_time = (esp_timer_get_time() / 1000UL) - output_state.motor_start_time;

    if ((esp_timer_get_time() / 1000UL) - output_state.motor_stop_time >
        Config.edge_delay + output_state.random_additional_delay) {
        time_out_over = ocTRUE;
    }

    // Ope, orgasm incoming! Stop it!
    if (!time_out_over) {
        orgasm_control_twitch_detect();

    } else if (arousal_state.arousal > Config.sensitivity_threshold &&
               output_state.motor_speed > 0 && on_time > Config.minimum_on_time) {
        // The motor_speed check above, btw, is so we only hit this once per peak.
        // Set the motor speed to 0, set stop time, and determine the new additional random time.
        output_state.motor_speed = controller->stop();
        output_state.motor_stop_time = (esp_timer_get_time() / 1000UL);
        output_state.motor_start_time = 0;
        arousal_state.update_flag = ocTRUE;

        event_manager_dispatch(EVT_ORGASM_DENIAL, NULL, 0);

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
        uint8_t speed = orgasm_control_get_motor_speed();
        event_manager_dispatch(EVT_SPEED_CHANGE, NULL, speed);
    }
}


// Orgasm modes State machine dispatching events to appropriate modules
// State is EVT_ driven 
static void orgasm_control_manage_orgasm_modes() {
    const vibration_mode_controller_t* orgasm_controller = &RampStopController;

    // Make sure menu_is_locked is turned off in Manual mode
    if (output_state.output_mode == OC_MANUAL_CONTROL) {
        orgasm_control.menu_is_locked = ocFALSE;
        orgasm_control.state = MANUAL;
        return;
    }

    if (output_state.output_mode == OC_AUTOMAITC_CONTROL) {
        orgasm_control.menu_is_locked = ocFALSE;
        orgasm_control.state = EDGING;
        return;
    }

    if (output_state.output_mode == OC_ORGASM_MODE){ 
        orgasm_controller->tick(output_state.motor_speed, arousal_state.arousal);

        if (orgasm_control.state == EDGING || orgasm_control.state == MANUAL) {
            // just started edging+orgams mode
            // initialize orgasm trigger and post orgasm modules
            orgasm_control.state = SESSION_SETUP;
        }

        // starting a egde-o-matic edging-orgasm mode
        // or received SESSION_SETUP EVT
        if (orgasm_control.state == SESSION_SETUP) {

            // Set the orgasm triger and post orgasm mode
            session_state.orgasm_trigger = orgasm_trigger_random_select(Config.orgasm_triggers);
            session_state.post_orgasm_mode = post_orgasm_mode_random_select(Config.post_orgasm_mode);

            orgasm_control.edging_start_millis = (esp_timer_get_time() / 1000UL);
            orgasm_control.state = SESSION_START;
        }

        // starting a egde-o-matic edging-orgasm mode
        // or received SESSION_START EVT
        if (orgasm_control.state == SESSION_START) {
            event_manager_dispatch(EVT_ORGASM_CONTROL_ORGASM_TRIGGER_SET, NULL, session_state.orgasm_trigger);
            event_manager_dispatch(EVT_ORGASM_CONTROL_POST_ORGASM_SET, NULL, session_state.post_orgasm_mode);
            if (!output_state.control_motor){
                orgasm_control_resume_control();    
            }
            orgasm_control.state = EDGING_ORGASM;
        }

        // Run the main edging loop until orgasm is permited
        if (orgasm_control.state == EDGING_ORGASM) {
            // RUN THE ORGASM TRIGGER MODULES
            event_manager_dispatch(EVT_ORGASM_CONTROL_IS_ORGASM_PERMITED, NULL, 0);
        
            // Update LED for Arousal Color
            float arousal_perc = orgasm_control_get_arousal_percent() * 255.0f;
            if (arousal_perc > 255.0f) arousal_perc = 255.0f;
            eom_hal_set_encoder_rgb(arousal_perc, 128 - (arousal_perc/2), 255 - arousal_perc);

            // Lock Menu if turned on. and in Edging_orgasm mode
            if (Config.edge_menu_lock && !orgasm_control.menu_is_locked) {
                // Lock only after 2 minutes
                if ((esp_timer_get_time() / 1000UL) >
                    orgasm_control.edging_start_millis + (2 * 60 * 1000)) {
                    orgasm_control.menu_is_locked = ocTRUE;
                    arousal_state.update_flag = ocTRUE;
                }
            }
        }
    
        // Orgasm is permit EVT was received
        // Controling Motor to give orgasm is controled by main program. This is not a module
        if (orgasm_control.state == ORGASM_IS_PERMITED) {
            if (output_state.control_motor) {
                orgasm_control_pause_control(); // make sure orgasm is now possible
                // set led green
                eom_hal_set_encoder_rgb(0, 255, 0);
            }

            update_check(output_state.motor_speed, orgasm_controller->increment() );
            event_manager_dispatch(EVT_SPEED_CHANGE, NULL, output_state.motor_speed);   
        }
        
        
        // Orgasm start EVT was detected
        // RUN POST ORGASM MODULES
        if (orgasm_control.state == POST_ORGASM) {
            
            event_manager_dispatch(EVT_ORGASM_CONTROL_POST_ORGASM_START, NULL, output_state.motor_speed);
            
            // Lock menu if turned on
            if (Config.post_orgasm_menu_lock && !orgasm_control.menu_is_locked) {
                orgasm_control.menu_is_locked = ocTRUE;
            }
            
            // Make sure GUI gets updated after post orgasm module has run
            arousal_state.update_flag = ocTRUE;
        }

        // Received a shutdown EVT request
        if (orgasm_control.state == SHUTDOWN) {
            // post orgasm modes has finished. Turn off everything and return to manual mode
            orgasm_control.menu_is_locked = ocFALSE;
            orgasm_control.state = MANUAL;
            update_check(output_state.motor_speed, 0 );
            event_manager_dispatch(EVT_SPEED_CHANGE, NULL, output_state.motor_speed);
            orgasm_control_set_output_mode(OC_MANUAL_CONTROL);            
        }
    }

}

/**
 *  Detect muscle clenching.  
 *  Used to ajust arousal if turned on
 *  Used to detect the start of a Orgasm
 *  @param p_check
 *  @return clench duration in ms
 */
long orgasm_control_clench_detect(long p_check){
    static bool orgasm_detect = false;
    // raise clench threshold to pressure - 1/2 sensitivity
    long current_time = (esp_timer_get_time() / 1000UL);
    if (p_check >=
        (clench_state.clench_pressure_threshold + Config.clench_pressure_sensitivity)) {
        clench_state.clench_pressure_threshold =
            (p_check - (Config.clench_pressure_sensitivity / 2));
    }

    // Start counting clench time if pressure over threshold
    if (p_check >= clench_state.clench_pressure_threshold) {
        clench_state.clench_duration_millis =
            current_time - clench_state.clench_start_millis;

        // Orgasm detected
        if (clench_state.clench_duration_millis >= Config.clench_time_to_orgasm_ms &&
            !orgasm_detect) {
            orgasm_detect = true;
            event_manager_dispatch(EVT_ORGASM_START, NULL, orgasm_control.orgasm_count + 1);    
        }
        return clench_state.clench_duration_millis;

    } else {
        orgasm_detect = false;
        clench_state.clench_start_millis = current_time;
        clench_state.clench_duration_millis -= 150; // ms
        if (clench_state.clench_duration_millis <= 0) {
            clench_state.clench_duration_millis = 0;
            // clench pressure threshold value decays over time to a min of pressure + 1/2
            // sensitivity
            if ((p_check + (Config.clench_pressure_sensitivity / 2)) <
                clench_state.clench_pressure_threshold) {
                clench_state.clench_pressure_threshold *= 0.99;
            }
        }
        return 0;
    } // END of clench detector
}

void orgasm_control_twitch_detect() {
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
            "millis,pressure,avg_pressure,arousal,motor_speed,sensitivity_threshold,"
            "clench_pressure_threshold,clench_duration"
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
        orgasm_control_manage_orgasm_modes();
        orgasm_control_updateMotorSpeed();
        arousal_state.last_update_ms = millis;

        // Data for logfile or classic log.
        char data_csv[255];
        snprintf(
            data_csv,
            255,
            "%d,%d,%d,%d,%ld,%ld,%d,%s",
            orgasm_control_get_average_pressure(),
            orgasm_control_get_arousal(),
            eom_hal_get_motor_speed(),
            Config.sensitivity_threshold,
            clench_state.clench_pressure_threshold,
            clench_state.clench_duration_millis,
            orgasm_control.orgasm_count,
            orgasm_control_state_str[orgasm_control.state]
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

void orgasm_control_permit_orgasm(int seconds) {
    orgasm_control_set_output_mode(OC_ORGASM_MODE);
    orgasm_control.state = ORGASM_IS_PERMITED;
    event_manager_dispatch(EVT_ORGASM_CONTROL_ORGASM_IS_PERMITED, NULL, seconds);
}

oc_bool_t orgasm_control_is_permit_orgasm_reached() {
    if (orgasm_control.state == ORGASM_IS_PERMITED) {
        return ocTRUE;
    } else {
        return ocFALSE;
    }
}

oc_bool_t orgasm_control_is_post_orgasm_reached() {
    // Detect if after orgasm
    if (orgasm_control.state == POST_ORGASM) {
        return ocTRUE;
    } else {
        return ocFALSE;
    }
}

oc_bool_t orgasm_control_is_menu_locked() {
    return orgasm_control.menu_is_locked;
};

void orgasm_control_lock_menu(oc_bool_t value) {
    orgasm_control.menu_is_locked = value;
    arousal_state.update_flag = ocTRUE;
}
