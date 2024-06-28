#include "assets/config_help.h"
#include "config.h"
#include "ui/input.h"
#include "menus/index.h"
#include "ui/menu.h"
#include "ui/ui.h"
#include "util/unit_str.h"


void on_config_save(int value, int final, UI_INPUT_ARG_TYPE arg);

void on_ui_reenter_menu_osm(int value, int final, UI_INPUT_ARG_TYPE arg){
    on_config_save(value,final,arg);
    ui_reenter_menu();
};

static const ui_input_numeric_t CLENCH_TIME_TO_ORGASM_MS_INPUT = {
    UnsignedInputValues(
        "Clench Time to Orgasm", &Config.clench_time_to_orgasm_ms, UNIT_MILLISECONDS, on_config_save
    ),
    .max = 10000, // 10000ms (10 sec) max to detect orgasm
    .step = 100,
    .input.help = CLENCH_TIME_TO_ORGASM_MS_HELP
};


static const ui_input_toggle_t USE_POST_ORGASM_INPUT = {
    ToggleInputValues("Enable Post Orgasm Mode", &Config.use_post_orgasm, on_ui_reenter_menu_osm),
    .input.help = USE_POST_ORGASM_HELP
};

static const ui_input_toggle_t EDGE_MENU_LOCK_INPUT = {
    ToggleInputValues("Menu Lock - Edge", &Config.edge_menu_lock, on_config_save),
    .input.help = EDGE_MENU_LOCK_HELP
};

static const ui_input_toggle_t CLENCH_DETECTOR_IN_EDGING_INPUT = {
    ToggleInputValues("Enable Clench in Edging", &Config.clench_detector_in_edging, on_config_save),
    .input.help = CLENCH_DETECTOR_IN_EDGING_HELP
};

static const ui_input_numeric_t CLENCH_PRESSURE_SENSITIVITY_INPUT = {
    UnsignedInputValues(
        "Clench Pressure Sensitivity", &Config.clench_pressure_sensitivity, "", on_config_save
    ),
    .max = 300,
    .input.help = CLENCH_PRESSURE_SENSITIVITY_HELP
};

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    ui_menu_add_input(m, (ui_input_t*)&USE_POST_ORGASM_INPUT);

    if (Config.use_post_orgasm) {
        ui_menu_add_menu(m, &ORGASM_TRIGGERS_MENU);
        ui_menu_add_menu(m, &POST_ORGASM_MENU);
        ui_menu_add_input(m, (ui_input_t*)&EDGE_MENU_LOCK_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&CLENCH_DETECTOR_IN_EDGING_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&CLENCH_TIME_TO_ORGASM_MS_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&CLENCH_PRESSURE_SENSITIVITY_INPUT);
    } else {
        ui_menu_add_input(m, (ui_input_t*)&CLENCH_DETECTOR_IN_EDGING_INPUT);
    }
}

DYNAMIC_MENU(ORGASM_SETTINGS_MENU, "Orgasm Settings", on_open);