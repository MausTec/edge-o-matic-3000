#include "assets/config_help.h"
#include "config.h"
#include "ui/input.h"
#include "ui/menu.h"
#include "util/unit_str.h"

void on_config_save(int value, int final, UI_INPUT_ARG_TYPE arg);

static const ui_input_numeric_t EDGING_DURATION_INPUT = {
    UnsignedInputValues(
        "Edge Duration Minutes", &Config.auto_edging_duration_minutes, UNIT_MINUTES, on_config_save
    ),
    .max = 300,
    .input.help = AUTO_EDGING_DURATION_MINUTES_HELP
};

static const ui_input_numeric_t POST_ORGASM_DURATION_SECONDS_INPUT = {
    UnsignedInputValues(
        "Post Orgasm Seconds", &Config.post_orgasm_duration_seconds, UNIT_SECONDS, on_config_save
    ),
    .max = 300,
    .input.help = POST_ORGASM_DURATION_SECONDS_HELP
};

static const ui_input_toggle_t USE_POST_ORGASM_INPUT = {
    ToggleInputValues("Use Post Orgasm Mode", &Config.use_post_orgasm, on_config_save),
    .input.help = USE_POST_ORGASM_HELP
};

static const ui_input_toggle_t EDGE_MENU_LOCK_INPUT = {
    ToggleInputValues("Menu Lock - Edge", &Config.edge_menu_lock, on_config_save),
    .input.help = EDGE_MENU_LOCK_HELP
};

static const ui_input_toggle_t POST_ORGASM_MENU_LOCK_INPUT = {
    ToggleInputValues("Menu Lock - Post", &Config.post_orgasm_menu_lock, on_config_save),
    .input.help = POST_ORGASM_MENU_LOCK_HELP
};

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    ui_menu_add_input(m, (ui_input_t*)&EDGING_DURATION_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&USE_POST_ORGASM_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&POST_ORGASM_DURATION_SECONDS_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&EDGE_MENU_LOCK_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&POST_ORGASM_MENU_LOCK_INPUT);
}

DYNAMIC_MENU(ORGASM_SETTINGS_MENU, "Orgasm Settings", on_open);