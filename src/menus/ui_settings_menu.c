#include "config.h"
#include "esp_log.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "util/fs.h"
#include "util/i18n.h"
#include <string.h>

static const char* TAG = "menu:ui_settings";

static void on_language_select(
    const struct ui_menu* m, const struct ui_menu_item* item, UI_MENU_ARG_TYPE menu_arg
) {
    ui_toast_blocking(_("Saving..."));
    const char* language = (const char*)item->arg;
    strlcpy(Config.language_file_name, language == NULL ? "" : language, CONFIG_PATH_MAX);
    config_enqueue_save(0);
    i18n_deinit();

    esp_err_t err = i18n_init();

    if (err != ESP_OK) {
        ui_toast("Invalid language file.");
    } else {
        ui_toast(_("Saved!"));
    }
}

static void _dir_result(const char* path, struct dirent* dir, void* arg) {
    if (arg == NULL || path == NULL || dir == NULL) return;
    if (fs_strcmp_ext(dir->d_name, ".json")) return;
    ui_menu_t* m = (ui_menu_t*)arg;

    ui_menu_item_t* item = ui_menu_add_item(m, dir->d_name, on_language_select, path);
    item->freer = free;
}

static void on_language_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    ui_menu_add_item(m, "English", on_language_select, NULL);
    fs_read_dir(fs_sd_root(), _dir_result, FS_READ_NO_HIDDEN | FS_READ_NO_FREE, m);
}

DYNAMIC_MENU(UI_LANGUAGE_MENU, "Language", on_language_open);

void on_config_save(int value, int final, UI_MENU_ARG_TYPE* arg) {
    if (final) {
        ui_toast(_("Saved!"));
        config_enqueue_save(0);
    }
}

static ui_input_numeric_t SCREEN_DIM_INPUT =
    UnsignedInput("Screen Dim Delay", &Config.screen_dim_seconds, "Seconds", on_config_save);
static ui_input_numeric_t SCREEN_TIMEOUT_INPUT =
    UnsignedInput("Screen Timeout", &Config.screen_timeout_seconds, "Seconds", on_config_save);
static ui_input_numeric_t LED_BRIGHTNESS_INPUT =
    UnsignedInput("LED Brightness", &Config.led_brightness, "%", on_config_save);
static ui_input_numeric_t SCREENSAVER_INPUT =
    ByteInput("Screensaver", &Config.enable_screensaver, "", on_config_save);

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    ui_menu_add_menu(m, &UI_LANGUAGE_MENU);
    ui_menu_add_input(m, (ui_input_t*)&SCREEN_DIM_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&SCREEN_TIMEOUT_INPUT);
    // ui_menu_add_input(m, (ui_input_t*)&LED_BRIGHTNESS_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&SCREENSAVER_INPUT);
}

DYNAMIC_MENU(UI_SETTINGS_MENU, "UI Settings", on_open);