#include "assets/config_help.h"
#include "config.h"
#include "ui/input.h"
#include "ui/menu.h"
#include "ui/ui.h"
#include "util/unit_str.h"
#include "post_orgasm_control.h"

void on_config_save(int value, int final, UI_INPUT_ARG_TYPE arg);

static const ui_input_numeric_t POST_ORGASM_DURATION_SECONDS_INPUT = {
    UnsignedInputValues(
        "Post Orgasm Seconds", &Config.post_orgasm_duration_seconds, UNIT_SECONDS, on_config_save
    ),
    .max = 300,
    .input.help = POST_ORGASM_DURATION_SECONDS_HELP
};

static const ui_input_numeric_t MOM_POST_ORGASM_DURATION_SECONDS_INPUT = {
    UnsignedInputValues(
        "MoM Post Orgasm Seconds", &Config.mom_post_orgasm_duration_seconds, UNIT_SECONDS, on_config_save
    ),
    .max = 300,
    .input.help = POST_ORGASM_DURATION_SECONDS_HELP
};

static const ui_input_toggle_t POST_ORGASM_MENU_LOCK_INPUT = {
    ToggleInputValues("Menu Lock - Post", &Config.post_orgasm_menu_lock, on_config_save),
    .input.help = POST_ORGASM_MENU_LOCK_HELP
};

void on_post_orgasm_select(const ui_input_select_option_t* option, int final, UI_INPUT_ARG_TYPE arg) {
    post_orgasm_mode_t mode = (post_orgasm_mode_t)option->ival;
    if (final) Config.post_orgasm_mode = mode;
    on_config_save(mode, final, arg);
    ui_reenter_menu();
}

static const ui_input_select_option_t post_orgasm_modes[] = {
    {
        .label = "Default Post orgasm",
        .ival = Default,
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
    {
        .label = "Ruin Orgasm",
        .ival = Ruin_orgasm,
        .value = NULL,
    }
};

static const ui_input_select_t POST_ORGASM_MODE_INPUT = {
    SelectInputValues(
        "Orgasm Modes",
        &Config.post_orgasm_mode,
        &post_orgasm_modes,
        sizeof(post_orgasm_modes) / sizeof(post_orgasm_modes[0]),
        on_post_orgasm_select
    ),
    .input.help = POST_ORGASM_MODE_HELP,
};

static const ui_input_numeric_t MILK_O_MATIC_REST_DURATION_INPUT = {
    UnsignedInputValues(
        "Milk-o-matic resting duration", &Config.milk_o_matic_rest_duration_seconds, UNIT_SECONDS, on_config_save
    ),
    .max = 43200,
    .step = 10,
    .input.help = "Resting time after a orgasm before restarting stimulation."
};

static const ui_input_numeric_t MAX_ORGASMS_INPUT = {
    UnsignedInputValues(
        "Milk-o-matic max orgasms", &Config.max_orgasms, "", on_config_save
    ),
    .max = 255,
    .step = 1,
    .input.help = "Orgasm count to stop milk-o-matic session"
};

static const ui_input_numeric_t DEFAULT_RANDOM_WEIGHT_INPUT = {
    UnsignedInputValues("Default Post Orgasm weight", &Config.random_post_default_weight, UNIT_PERCENT, on_config_save),
    .max = 100,
    .step = 1,
    .input.help = "Percent chance this trigger is selected."
};

static const ui_input_numeric_t MOM_RANDOM_WEIGHT_INPUT = {
    UnsignedInputValues("Milk-O-Matic weight", &Config.random_post_mom_weight, UNIT_PERCENT, on_config_save),
    .max = 100,
    .step = 1,
    .input.help = "Percent chance this trigger is selected."
};

static const ui_input_numeric_t RUIN_RANDOM_WEIGHT_INPUT = {
    UnsignedInputValues("Ruin Orgasm weight", &Config.random_post_ruin_weight, UNIT_PERCENT, on_config_save),
    .max = 100,
    .step = 1,
    .input.help = "Percent chance this trigger is selected."
};

static const ui_input_toggle_t POST_ORGASM_SESSION_RESTART_SETUP_INPUT = {
    ToggleInputValues("Post Orgasm - Restart setup", &Config.orgasm_session_setup_post_default, on_config_save),
    .input.help = "Restart a session from setup - usefull in random choice setups"
};
static const ui_input_toggle_t MOM_SESSION_RESTART_SETUP_INPUT = {
    ToggleInputValues("M-o-M - Restart setup", &Config.orgasm_session_setup_m_o_m, on_config_save),
    .input.help = "Restart a session from setup - usefull in random choice setups"
};
static const ui_input_toggle_t RUIN_SESSION_RESTART_SETUP_INPUT = {
    ToggleInputValues("Ruin - Restart setup", &Config.orgasm_session_setup_ruin, on_config_save),
    .input.help = "Restart a session from setup - usefull in random choice setups"
};

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    ui_menu_add_input(m, (ui_input_t*)&POST_ORGASM_MODE_INPUT);

    if (Config.post_orgasm_mode == Default) {
        ui_menu_add_input(m, (ui_input_t*)&POST_ORGASM_DURATION_SECONDS_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&POST_ORGASM_MENU_LOCK_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&POST_ORGASM_SESSION_RESTART_SETUP_INPUT);
    }

    if (Config.post_orgasm_mode == Ruin_orgasm) {
        ui_menu_add_input(m, (ui_input_t*)&RUIN_SESSION_RESTART_SETUP_INPUT);
    }

    if (Config.post_orgasm_mode == Milk_o_matic) {
        ui_menu_add_input(m, (ui_input_t*)&MOM_POST_ORGASM_DURATION_SECONDS_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&POST_ORGASM_MENU_LOCK_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&MILK_O_MATIC_REST_DURATION_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&MAX_ORGASMS_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&MOM_SESSION_RESTART_SETUP_INPUT);
    }    

    if (Config.post_orgasm_mode == Random_mode) {
        ui_menu_add_input(m, (ui_input_t*)&POST_ORGASM_DURATION_SECONDS_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&MOM_POST_ORGASM_DURATION_SECONDS_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&POST_ORGASM_MENU_LOCK_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&MILK_O_MATIC_REST_DURATION_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&MAX_ORGASMS_INPUT);

        ui_menu_add_input(m, (ui_input_t*)&DEFAULT_RANDOM_WEIGHT_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&MOM_RANDOM_WEIGHT_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&RUIN_RANDOM_WEIGHT_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&POST_ORGASM_SESSION_RESTART_SETUP_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&RUIN_SESSION_RESTART_SETUP_INPUT);
        ui_menu_add_input(m, (ui_input_t*)&MOM_SESSION_RESTART_SETUP_INPUT);
    }
}

DYNAMIC_MENU(POST_ORGASM_MENU, "Post Orgasm modes", on_open);