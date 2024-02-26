#include "assets/config_help.h"
#include "config.h"
#include "esp_log.h"
#include "ui/menu.h"
#include "util/unit_str.h"
#include <math.h>

static const char* TAG = "eding_settings_menu";

void on_config_save(int value, int final, UI_INPUT_ARG_TYPE arg);

void on_vibrate_select(const ui_input_select_option_t* option, int final, UI_INPUT_ARG_TYPE arg) {
    vibration_mode_t mode = (vibration_mode_t)option->ival;
    if (final) Config.vibration_mode = mode;
    on_config_save(mode, final, arg);
}

static const ui_input_select_option_t vibration_modes[] = {
    {
        .label = "Depletion",
        .ival = Depletion,
        .value = NULL,
    },
    {
        .label = "Enhancement",
        .ival = Enhancement,
        .value = NULL,
    },
    {
        .label = "Ramp / Stop",
        .ival = RampStop,
        .value = NULL,
    },
};

static const ui_input_select_t VIBRATION_MODE_INPUT = {
    SelectInputValues(
        "Vibration Mode",
        &Config.vibration_mode,
        &vibration_modes,
        sizeof(vibration_modes) / sizeof(vibration_modes[0]),
        on_vibrate_select
    ),
    .input.help = VIBRATION_MODE_HELP,
};

static const ui_input_byte_t MOTOR_MAX_SPEED_INPUT = {
    ByteInputValues("Motor Max Speed", &Config.motor_max_speed, UNIT_PERCENT, on_config_save),
    .input.help = MOTOR_MAX_SPEED_HELP
};

static const ui_input_byte_t MOTOR_START_SPEED_INPUT = {
    ByteInputValues("Motor Start Speed", &Config.motor_start_speed, UNIT_PERCENT, on_config_save),
    .input.help = MOTOR_START_SPEED_HELP
};

static const ui_input_numeric_t MOTOR_RAMP_TIME_INPUT = {
    UnsignedInputValues("Motor Ramp Time", &Config.motor_ramp_time_s, UNIT_SECONDS, on_config_save),
    .max = 120,
    .step = 5,
    .input.help = MOTOR_RAMP_TIME_S_HELP
};

static const ui_input_numeric_t EDGE_DELAY_INPUT = {
    UnsignedInputValues("Edge Delay", &Config.edge_delay, UNIT_MILLISECONDS, on_config_save),
    .step = 100,
    .input.help = EDGE_DELAY_HELP
};

static const ui_input_numeric_t MAX_ADDITIONAL_DELAY_INPUT = {
    UnsignedInputValues(
        "Max Additional Delay", &Config.max_additional_delay, UNIT_MILLISECONDS, on_config_save
    ),
    .max = 6000,
    .step = 100,
    .input.help = MAX_ADDITIONAL_DELAY_HELP
};

static const ui_input_numeric_t MINIMUM_ON_TIME_INPUT = {
    UnsignedInputValues(
        "Minimum On Time", &Config.minimum_on_time, UNIT_MILLISECONDS, on_config_save
    ),
    .max = 5000,
    .step = 50,
    .input.help = MINIMUM_ON_TIME_HELP
};

static const ui_input_numeric_t AROUSAL_LIMIT_INPUT = {
    UnsignedInputValues("Arousal Limit", &Config.sensitivity_threshold, "", on_config_save),
    .max = 1023,
    .input.help = SENSITIVITY_THRESHOLD_HELP
};

static int get_sensor_reading(int sens, int final, UI_INPUT_ARG_TYPE arg) {
    static int last_setting = 0;

    if (sens != last_setting) {
        eom_hal_set_sensor_sensitivity((uint8_t)sens);
        last_setting = sens;
    }

    uint16_t reading = eom_hal_get_pressure_reading();
    return floor(255.0f * ((float)reading / EOM_HAL_PRESSURE_MAX));
}

// TODO: This no longer shows a secondary chart, but it could.
static const ui_input_byte_t SENSOR_SENSITIVITY_INPUT = {
    ByteInputValues("Sensor Sensitivity", &Config.sensor_sensitivity, "%", on_config_save),
    .input.help = SENSOR_SENSITIVITY_HELP,
    .chart_getter = get_sensor_reading
};

static const ui_input_toggle_t USE_DENIAL_COUNT_IN_SENSITIVITY_INPUT = {
    ToggleInputValues("Use Denial Count in Sensitivity", &Config.use_denial_count_in_sensitivity, on_config_save),
    .input.help = USE_DENIAL_COUNT_IN_SENSITIVITY_HELP
};

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    ui_menu_add_input(m, (ui_input_t*)&VIBRATION_MODE_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&MOTOR_MAX_SPEED_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&MOTOR_START_SPEED_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&MOTOR_RAMP_TIME_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&EDGE_DELAY_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&MAX_ADDITIONAL_DELAY_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&MINIMUM_ON_TIME_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&AROUSAL_LIMIT_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&SENSOR_SENSITIVITY_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&USE_DENIAL_COUNT_IN_SENSITIVITY_INPUT);
}

DYNAMIC_MENU(EDGING_SETTINGS_MENU, "Edging Settings", on_open);