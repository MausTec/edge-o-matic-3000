#include "assets/config_help.h"
#include "config.h"
#include "ui/input.h"
#include "ui/menu.h"
#include "util/unit_str.h"

void on_config_save(int value, int final, UI_INPUT_ARG_TYPE arg);

void on_to_orgasm_select(const ui_input_select_option_t* option, int final, UI_INPUT_ARG_TYPE arg) {
    to_orgasm_mode_t mode = (to_orgasm_mode_t)option->ival;
    if (final) Config.to_orgasm_mode = mode;
    on_config_save(mode, final, arg);
}

static const ui_input_select_option_t to_orgasm_modes[] = {
    {
        .label = "Timer",
        .ival = Timer,
        .value = NULL,
    },
    {
        .label = "Denial count",
        .ival = Denial_count,
        .value = NULL,
    },
    {
        .label = "Random choice",
        .ival = Random_mode,
        .value = NULL,
    },
    {
        .label = "Milk-O-Matic",
        .ival = Milk_o_matic,
        .value = NULL,
    },
};

static const ui_input_select_t TO_ORGASM_MODE_INPUT = {
    SelectInputValues(
        "Orgasm Modes",
        &Config.to_orgasm_mode,
        &to_orgasm_modes,
        sizeof(to_orgasm_modes) / sizeof(to_orgasm_modes[0]),
        on_to_orgasm_select
    ),
    .input.help = TO_ORGASM_MODE_HELP,
};

static const ui_input_numeric_t EDGING_DURATION_INPUT = {
    UnsignedInputValues(
        "Edge Duration Minutes", &Config.auto_edging_duration_minutes, UNIT_MINUTES, on_config_save
    ),
    .max = 300,
    .input.help = AUTO_EDGING_DURATION_MINUTES_HELP
};

static const ui_input_numeric_t CLENCH_TIME_TO_ORGASM_MS_INPUT = {
    UnsignedInputValues(
        "Clench Time to Orgasm", &Config.clench_time_to_orgasm_ms, UNIT_MILLISECONDS, on_config_save
    ),
    .max = 5000, // 5000ms (5 sec) max to detect orgasm
    .step = 100,
    .input.help = CLENCH_TIME_TO_ORGASM_MS_HELP
};

static const ui_input_numeric_t POST_ORGASM_DURATION_SECONDS_INPUT = {
    UnsignedInputValues(
        "Post Orgasm Seconds", &Config.post_orgasm_duration_seconds, UNIT_SECONDS, on_config_save
    ),
    .max = 300,
    .input.help = POST_ORGASM_DURATION_SECONDS_HELP
};

static const ui_input_toggle_t USE_ORGASM_MODES_INPUT = {
    ToggleInputValues("Enable Orgasm Modes", &Config.use_orgasm_modes, on_config_save),
    .input.help = USE_ORGASM_MODES_HELP
};

static const ui_input_toggle_t EDGE_MENU_LOCK_INPUT = {
    ToggleInputValues("Menu Lock - Edge", &Config.edge_menu_lock, on_config_save),
    .input.help = EDGE_MENU_LOCK_HELP
};

static const ui_input_toggle_t POST_ORGASM_MENU_LOCK_INPUT = {
    ToggleInputValues("Menu Lock - Post", &Config.post_orgasm_menu_lock, on_config_save),
    .input.help = POST_ORGASM_MENU_LOCK_HELP
};

static const ui_input_toggle_t CLENCH_DETECTOR_IN_EDGING_INPUT = {
    ToggleInputValues("Use Clench in Edging", &Config.clench_detector_in_edging, on_config_save),
    .input.help = CLENCH_DETECTOR_IN_EDGING_HELP
};

static const ui_input_numeric_t CLENCH_PRESSURE_SENSITIVITY_INPUT = {
    UnsignedInputValues(
        "Clench Pressure Sensitivity", &Config.clench_pressure_sensitivity, "", on_config_save
    ),
    .max = 300,
    .input.help = CLENCH_PRESSURE_SENSITIVITY_HELP
};

static const ui_input_numeric_t DENIALS_COUNT_TO_ORGASM_INPUT = {
    UnsignedInputValues("Denials to permit orgasm", &Config.denials_count_to_orgasm, "", on_config_save),
    .max = 255,
    .step = 1,
    .input.help = DENIALS_COUNT_TO_ORGASM_HELP
};

static const ui_input_numeric_t MILK_O_MATIC_REST_DURATION_INPUT = {
    UnsignedInputValues(
        "MILK-O-MATIC rest duration", &Config.milk_o_matic_rest_duration_seconds, UNIT_SECONDS, on_config_save
    ),
    .max = 1800,
    .step = 1,
    .input.help = MILK_O_MATIC_REST_DURATION_SECONDS_HELP
};

static const ui_input_toggle_t RANDOM_ORGASM_TRIGGERS_INPUT = {
    ToggleInputValues("Random Timer and Edge_count", &Config.random_orgasm_triggers, on_config_save),
    .input.help = RANDOM_ORGASM_TRIGGERS_HELP
};

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    ui_menu_add_input(m, (ui_input_t*)&USE_ORGASM_MODES_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&TO_ORGASM_MODE_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&DENIALS_COUNT_TO_ORGASM_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&EDGING_DURATION_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&CLENCH_TIME_TO_ORGASM_MS_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&POST_ORGASM_DURATION_SECONDS_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&EDGE_MENU_LOCK_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&POST_ORGASM_MENU_LOCK_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&CLENCH_DETECTOR_IN_EDGING_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&CLENCH_PRESSURE_SENSITIVITY_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&MILK_O_MATIC_REST_DURATION_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&RANDOM_ORGASM_TRIGGERS_INPUT);
}

DYNAMIC_MENU(ORGASM_SETTINGS_MENU, "Orgasm Settings", on_open);