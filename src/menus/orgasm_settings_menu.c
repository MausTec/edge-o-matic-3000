#include "assets/config_help.h"
#include "config.h"
#include "esp_log.h"
#include "ui/menu.h"
#include "util/unit_str.h"

static const char* TAG = "orgasm_settings_menu";

void on_config_save(int value, int final, UI_INPUT_ARG_TYPE arg);

// ---- Detection Mode Select

void on_detect_mode_select(
    const ui_input_select_option_t* option, int final, UI_INPUT_ARG_TYPE arg
) {
    if (final) Config.od_mode = option->ival;
    on_config_save(option->ival, final, arg);
}

static const ui_input_select_option_t detect_modes[] = {
    { .label = "Auto", .ival = OD_MODE_AUTO, .value = NULL },
    { .label = "Rhythmic Only", .ival = OD_MODE_RHYTHMIC, .value = NULL },
    { .label = "Sustained Only", .ival = OD_MODE_SUSTAINED, .value = NULL },
};

static const ui_input_select_t DETECT_MODE_INPUT = {
    SelectInputValues(
        "Detection Mode",
        &Config.od_mode,
        &detect_modes,
        sizeof(detect_modes) / sizeof(detect_modes[0]),
        on_detect_mode_select
    ),
    .input.help = OD_MODE_HELP,
};

// ---- Toggle Inputs

static const ui_input_toggle_t DETECTION_ARMED_INPUT = {
    ToggleInputValues("Detection Armed", &Config.od_detection_armed, on_config_save),
    .input.help = OD_DETECTION_ARMED_HELP,
};

static const ui_input_toggle_t CLENCH_BOOST_INPUT = {
    ToggleInputValues("Clench Arousal Boost", &Config.od_clench_arousal_boost, on_config_save),
    .input.help = OD_CLENCH_AROUSAL_BOOST_HELP,
};

// ---- Numeric Inputs

static const ui_input_numeric_t AROUSAL_DECAY_RATE_INPUT = {
    UnsignedInputValues("Arousal Decay Rate", &Config.arousal_decay_rate, "%", on_config_save),
    .max = 99,
    .step = 5,
    .input.help = AROUSAL_DECAY_RATE_HELP,
};

static const ui_input_numeric_t SUSTAINED_THRESHOLD_INPUT = {
    UnsignedInputValues("Sustained Threshold", &Config.od_sustained_threshold, "", on_config_save),
    .max = 2048,
    .step = 10,
    .input.help = OD_SUSTAINED_THRESHOLD_HELP,
};

static const ui_input_numeric_t SUSTAINED_FALLBACK_INPUT = {
    UnsignedInputValues(
        "Sustained Fallback", &Config.od_sustained_fallback_ms, UNIT_MILLISECONDS, on_config_save
    ),
    .max = 30000,
    .step = 500,
    .input.help = OD_SUSTAINED_FALLBACK_MS_HELP,
};

static const ui_input_numeric_t SUSTAINED_DROPOUT_INPUT = {
    UnsignedInputValues(
        "Sustained Dropout", &Config.od_sustained_dropout_ms, UNIT_MILLISECONDS, on_config_save
    ),
    .max = 5000,
    .step = 100,
    .input.help = OD_SUSTAINED_DROPOUT_MS_HELP,
};

static const ui_input_numeric_t PEAK_MIN_AMPLITUDE_INPUT = {
    UnsignedInputValues("Peak Min Amplitude", &Config.od_peak_min_amplitude, "", on_config_save),
    .max = 500,
    .step = 5,
    .input.help = OD_PEAK_MIN_AMPLITUDE_HELP,
};

static const ui_input_numeric_t RHYTHMIC_MIN_PEAKS_INPUT = {
    UnsignedInputValues("Rhythmic Min Peaks", &Config.od_rhythmic_min_peaks, "", on_config_save),
    .max = 15,
    .step = 1,
    .input.help = OD_RHYTHMIC_MIN_PEAKS_HELP,
};

static const ui_input_numeric_t RHYTHMIC_INTERVAL_MIN_INPUT = {
    UnsignedInputValues(
        "Rhythm Interval Min",
        &Config.od_rhythmic_interval_min_ms,
        UNIT_MILLISECONDS,
        on_config_save
    ),
    .max = 2000,
    .step = 50,
    .input.help = OD_RHYTHMIC_INTERVAL_MIN_MS_HELP,
};

static const ui_input_numeric_t RHYTHMIC_INTERVAL_MAX_INPUT = {
    UnsignedInputValues(
        "Rhythm Interval Max",
        &Config.od_rhythmic_interval_max_ms,
        UNIT_MILLISECONDS,
        on_config_save
    ),
    .max = 3000,
    .step = 50,
    .input.help = OD_RHYTHMIC_INTERVAL_MAX_MS_HELP,
};

static const ui_input_numeric_t RHYTHMIC_VARIANCE_INPUT = {
    UnsignedInputValues(
        "Rhythm Variance",
        &Config.od_rhythmic_interval_variance_ms,
        UNIT_MILLISECONDS,
        on_config_save
    ),
    .max = 500,
    .step = 25,
    .input.help = OD_RHYTHMIC_INTERVAL_VARIANCE_MS_HELP,
};

static const ui_input_numeric_t RHYTHMIC_TIMEOUT_INPUT = {
    UnsignedInputValues(
        "Rhythm Timeout", &Config.od_rhythmic_timeout_ms, UNIT_MILLISECONDS, on_config_save
    ),
    .max = 5000,
    .step = 100,
    .input.help = OD_RHYTHMIC_TIMEOUT_MS_HELP,
};

static const ui_input_numeric_t AROUSAL_GATE_INPUT = {
    UnsignedInputValues("Arousal Gate", &Config.od_arousal_gate_percent, "%", on_config_save),
    .max = 100,
    .step = 5,
    .input.help = OD_AROUSAL_GATE_PERCENT_HELP,
};

static const ui_input_numeric_t RECOVERY_MS_INPUT = {
    UnsignedInputValues("Recovery Time", &Config.od_recovery_ms, UNIT_MILLISECONDS, on_config_save),
    .max = 10000,
    .step = 500,
    .input.help = OD_RECOVERY_MS_HELP,
};

static const ui_input_numeric_t CLENCH_BOOST_AMOUNT_INPUT = {
    UnsignedInputValues("Boost Amount", &Config.od_clench_arousal_boost_amount, "", on_config_save),
    .max = 50,
    .step = 1,
    .input.help = OD_CLENCH_AROUSAL_BOOST_AMOUNT_HELP,
};

// ---- Menu

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    // Core settings (most commonly adjusted)
    ui_menu_add_input(m, (ui_input_t*)&DETECT_MODE_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&DETECTION_ARMED_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&AROUSAL_DECAY_RATE_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&AROUSAL_GATE_INPUT);

    // Sustained detection settings
    ui_menu_add_input(m, (ui_input_t*)&SUSTAINED_THRESHOLD_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&SUSTAINED_FALLBACK_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&SUSTAINED_DROPOUT_INPUT);

    // Rhythmic detection settings
    ui_menu_add_input(m, (ui_input_t*)&PEAK_MIN_AMPLITUDE_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&RHYTHMIC_MIN_PEAKS_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&RHYTHMIC_INTERVAL_MIN_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&RHYTHMIC_INTERVAL_MAX_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&RHYTHMIC_VARIANCE_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&RHYTHMIC_TIMEOUT_INPUT);

    // Recovery and boost
    ui_menu_add_input(m, (ui_input_t*)&RECOVERY_MS_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&CLENCH_BOOST_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&CLENCH_BOOST_AMOUNT_INPUT);
}

/**
 * @brief You asked for better orgasm settings, you get better orgasm settings.
 * 
 * This menu is a playground for testing new orgasm detection features, so a lot of different
 * experimental parameters were exposed. Consider it "Advanced".
 */
DYNAMIC_MENU(ORGASM_SETTINGS_MENU, "Orgasm Settings", on_open);
