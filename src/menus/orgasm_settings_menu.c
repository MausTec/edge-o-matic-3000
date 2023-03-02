#include "ui/ui.h"
#include "util/unit_str.h"

void on_config_save(int value, int final, UI_MENU_ARG_TYPE* arg);

static const ui_input_numeric_t EDGING_DURATION_INPUT = {};
static const ui_input_numeric_t CLENCH_ORGASM_TIME_THRESHOLD_INPUT = {};
static const ui_input_toggle_t EDGE_MENU_LOCK_INPUT = {};
static const ui_input_toggle_t POST_ORGASM_MENU_LOCK_INPUT = {};
static const ui_input_toggle_t CLENCH_DETECTOR_IN_EDGING_INPUT = {};
static const ui_input_numeric_t CLENCH_PRESSURE_SENSITIVITY_INPUT = {};

static void on_open(const ui_menu_t* m, const void* arg) {
    ui_menu_add_input(m, &EDGING_DURATION_INPUT);
    ui_menu_add_input(m, &CLENCH_ORGASM_TIME_THRESHOLD_INPUT);
    ui_menu_add_input(m, &EDGE_MENU_LOCK_INPUT);
    ui_menu_add_input(m, &POST_ORGASM_MENU_LOCK_INPUT);
    ui_menu_add_input(m, &CLENCH_DETECTOR_IN_EDGING_INPUT);
    ui_menu_add_input(m, &CLENCH_PRESSURE_SENSITIVITY_INPUT);
}

DYNAMIC_MENU(ORGASM_SETTINGS_MENU, "Orgasm Settings", on_open);