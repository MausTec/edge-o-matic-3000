#include "config.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "util/i18n.h"
#include <string.h>

static void on_language_select(
    const struct ui_menu* m, const struct ui_menu_item* item, UI_MENU_ARG_TYPE menu_arg
) {
    const char* language = (const char*)item->arg;
    strlcpy(Config.language_file_name, language == NULL ? "" : language, CONFIG_PATH_MAX);
    config_enqueue_save(0);
    i18n_deinit();
    i18n_init();
}

static void on_language_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    ui_menu_add_item(m, "English", on_language_select, NULL);
}

DYNAMIC_MENU(UI_LANGUAGE_MENU, "Language", on_language_open);

static void on_config_save(int value, int final, UI_MENU_ARG_TYPE* arg) {
    if (final) {
        ui_toast(_("Saved!"));
        config_enqueue_save(0);
    }
}

// Main

static ui_input_numeric_t SCREEN_DIM_INPUT =
    UnsignedInput("Screen Dim Delay", &Config.screen_dim_seconds, "Seconds", on_config_save);
static ui_input_numeric_t SCREEN_TIMEOUT_INPUT =
    UnsignedInput("Screen Timeout", &Config.screen_timeout_seconds, "Seconds", on_config_save);
static ui_input_numeric_t LED_BRIGHTNESS_INPUT =
    UnsignedInput("LED Brightness", &Config.led_brightness, "%", on_config_save);

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    ui_menu_add_menu(m, &UI_LANGUAGE_MENU);
    ui_menu_add_input(m, &SCREEN_DIM_INPUT);
    ui_menu_add_input(m, &SCREEN_TIMEOUT_INPUT);
    ui_menu_add_input(m, &LED_BRIGHTNESS_INPUT);
}

DYNAMIC_MENU(UI_SETTINGS_MENU, "UI Settings", on_open);