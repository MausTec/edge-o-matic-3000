#include "assets/config_help.h"
#include "config.h"
#include "ui/input.h"
#include "ui/menu.h"
#include "ui/ui.h"
#include "util/unit_str.h"
#include "util/unit_str.h"
#include "orgasm_trigger_control.h"

void on_config_save(int value, int final, UI_INPUT_ARG_TYPE arg);

void on_ui_reenter_menu_otm(int value, int final, UI_INPUT_ARG_TYPE arg){
    on_config_save(value,final,arg);
    ui_reenter_menu();
};

static const ui_input_numeric_t EDGING_DURATION_MIN_INPUT = {
    UnsignedInputValues(
        "Min Edging Seconds - for random", &Config.min_timer_seconds, UNIT_SECONDS, on_config_save
    ),
    .max = 43200,
    .step = 10,
    .input.help = "Random will calculate between this minimum and Edging Duration Seconds value."
};

static const ui_input_numeric_t EDGING_DURATION_MAX_INPUT = {
    UnsignedInputValues(
        "Edging Duration Seconds", &Config.max_timer_seconds, UNIT_SECONDS, on_config_save
    ),
    .max = 43200,
    .step = 10,
    .input.help = AUTO_EDGING_DURATION_MINUTES_HELP
};

static const ui_input_numeric_t EDGES_COUNT_TO_ORGASM_MIN_INPUT = {
    UnsignedInputValues("Min Edges - for random", &Config.min_edge_count, "", on_config_save),
    .max = 1000,
    .step = 1,
    .input.help = "Random will calculate between this minimum and Edges to permit orgasm value."
};

static const ui_input_numeric_t EDGES_COUNT_TO_ORGASM_MAX_INPUT = {
    UnsignedInputValues("Edges to permit orgasm", &Config.max_edge_count, "", on_config_save),
    .max = 1000,
    .step = 1,
    .input.help = "How many edges before permiting orgasm."
};


static const ui_input_toggle_t RANDOM_ORGASM_TRIGGERS_INPUT = {
    ToggleInputValues("Enable Random permit value", &Config.random_orgasm_triggers, on_ui_reenter_menu_otm),
    .input.help = "Permit Orgasm trigger value is calculated between min and max."
};

static const ui_input_numeric_t TIMER_RANDOM_WEIGHT_INPUT = {
    UnsignedInputValues("Timer weight", &Config.random_trigger_timer_weight, UNIT_PERCENT, on_config_save),
    .max = 100,
    .step = 1,
    .input.help = "Percent chance this trigger is selected."
};

static const ui_input_numeric_t EDGE_RANDOM_WEIGHT_INPUT = {
    UnsignedInputValues("Edge weight", &Config.random_trigger_edge_count_weight, UNIT_PERCENT, on_config_save),
    .max = 100,
    .step = 1,
    .input.help = "Percent chance this trigger is selected."
};

static const ui_input_numeric_t NOW_RANDOM_WEIGHT_INPUT = {
    UnsignedInputValues("Orgasm Now weight", &Config.random_trigger_now_weight, UNIT_PERCENT, on_config_save),
    .max = 100,
    .step = 1,
    .input.help = "Percent chance this trigger is selected."
};

void on_orgasm_triggers_select(const ui_input_select_option_t* option, int final, UI_INPUT_ARG_TYPE arg) {
    const ui_menu_t* m = (const ui_menu_t*)arg;
    orgasm_triggers_t mode = (orgasm_triggers_t)option->ival;
    if (final) Config.orgasm_triggers = mode;
    on_ui_reenter_menu_otm(mode, final, arg);
}

static ui_input_select_option_t orgasm_triggers[] = {
    {
        .label = "Default Timer",
        .ival = Timer,
        .value = NULL,
    },
    {
        .label = "Edge count",
        .ival = Edge_count,
        .value = NULL,
    },
    {
        .label = "Now",
        .ival = Now,
        .value = NULL,
    },    {
        .label = "Random trigger",
        .ival = Random_triggers,
        .value = NULL,
    },
};

static ui_input_select_t ORGASM_TRIGGERS_INPUT = {
    SelectInputValues(
        "Orgasm Triggers",
        &Config.orgasm_triggers,
        &orgasm_triggers,
        sizeof(orgasm_triggers) / sizeof(orgasm_triggers[0]),
        on_orgasm_triggers_select
    ),
    .input.help = "Choose which orgasm trigger to use in Orgasm mode."
};

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    ui_menu_add_input(m, (ui_input_t*)&ORGASM_TRIGGERS_INPUT);

    if (Config.orgasm_triggers == Timer) {
        ui_menu_add_input(m, (ui_input_t*)&EDGING_DURATION_MAX_INPUT);
        if (Config.random_orgasm_triggers) {
            ui_menu_add_input(m, (ui_input_t*)&EDGING_DURATION_MIN_INPUT);
        }
    }

    if (Config.orgasm_triggers == Edge_count) {
        ui_menu_add_input(m, (ui_input_t*)&EDGES_COUNT_TO_ORGASM_MAX_INPUT);
        if (Config.random_orgasm_triggers) {
            ui_menu_add_input(m, (ui_input_t*)&EDGES_COUNT_TO_ORGASM_MIN_INPUT);
        }
    }    

    if (Config.orgasm_triggers == Random_triggers) {
        ui_menu_add_input(m, (ui_input_t*)&EDGING_DURATION_MAX_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&EDGES_COUNT_TO_ORGASM_MAX_INPUT);
        if (Config.random_orgasm_triggers) {
            ui_menu_add_input(m, (ui_input_t*)&EDGING_DURATION_MIN_INPUT);
            ui_menu_add_input(m, (ui_input_t*)&EDGES_COUNT_TO_ORGASM_MIN_INPUT);
        }
        ui_menu_add_input(m, (ui_input_t*)&TIMER_RANDOM_WEIGHT_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&EDGE_RANDOM_WEIGHT_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&NOW_RANDOM_WEIGHT_INPUT);
    }

    if (Config.orgasm_triggers != Now) {
        ui_menu_add_input(m, (ui_input_t*)&RANDOM_ORGASM_TRIGGERS_INPUT);
    }

}

DYNAMIC_MENU(ORGASM_TRIGGERS_MENU, "Orgasm Triggers", on_open);