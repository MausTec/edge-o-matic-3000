#include "assets/config_help.h"
#include "config.h"
#include "esp_log.h"
#include "menus/index.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "util/fs.h"
#include "util/i18n.h"
#include <string.h>

static const char* TAG = "menu:ui_settings";

void on_config_save(int value, int final, UI_INPUT_ARG_TYPE arg) {
    if (final) {
        if (eom_hal_get_sd_size_bytes() > 0) {
            ui_toast_blocking(_("Saving..."));
            config_enqueue_save(0);
            ui_toast(_("Saved!"));
        } else {
            ui_toast(_("Temporary Change, no SD card inserted."));
        }
    }
}

static const ui_input_numeric_t SCREEN_DIM_INPUT = {
    UnsignedInputValues("Screen Dim Delay", &Config.screen_dim_seconds, "Seconds", on_config_save),
    .input.help = SCREEN_DIM_SECONDS_HELP,
};

static const ui_input_numeric_t SCREEN_TIMEOUT_INPUT = {
    UnsignedInputValues(
        "Screen Timeout", &Config.screen_timeout_seconds, "Seconds", on_config_save
    ),
    .input.help = SCREEN_TIMEOUT_SECONDS_HELP,
};

static const ui_input_byte_t LED_BRIGHTNESS_INPUT = {
    ByteInputValues("LED Brightness", &Config.led_brightness, "%", on_config_save),
    .input.help = LED_BRIGHTNESS_HELP,
};

static const ui_input_toggle_t SCREENSAVER_INPUT = {
    ToggleInputValues("Screensaver", &Config.enable_screensaver, on_config_save), .input.help = NULL
};

static const ui_input_toggle_t REVERSE_MENU_SCROLL_INPUT = {
    ToggleInputValues("Reverse Menu Scroll", &Config.reverse_menu_scroll, on_config_save),
    .input.help = REVERSE_MENU_SCROLL_HELP,
};

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    ui_menu_add_menu(m, &UI_LANGUAGE_MENU);
    ui_menu_add_input(m, (ui_input_t*)&SCREEN_DIM_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&SCREEN_TIMEOUT_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&LED_BRIGHTNESS_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&SCREENSAVER_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&REVERSE_MENU_SCROLL_INPUT);
}

DYNAMIC_MENU(UI_SETTINGS_MENU, "UI Settings", on_open);