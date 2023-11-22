#include "cJSON.h"
#include "config.h"
#include "esp_log.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "util/fs.h"
#include "util/i18n.h"
#include <string.h>

static const char* TAG = "menu:ui_language";

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

struct dir_result_arg {
    const char* path;
    int depth;
    const ui_menu_t* menu;
};

static void _dir_result(const char* path, struct dirent* dir, void* argp) {
    if (argp == NULL || path == NULL || dir == NULL) return;
    struct dir_result_arg* arg = (struct dir_result_arg*)argp;
    char full_path[CONFIG_PATH_MAX + 1];

    snprintf(full_path, CONFIG_PATH_MAX, "%s/%s", arg->path, path);

    if (dir->d_type == DT_DIR) {
        if (arg->depth > 1) return;
        ESP_LOGI(TAG, "Read Directory Recursive: %s/", full_path);

        struct dir_result_arg dir_arg = {
            .path = full_path,
            .depth = arg->depth + 1,
            .menu = arg->menu,
        };

        fs_read_dir(full_path, _dir_result, FS_READ_NO_HIDDEN | FS_READ_NO_FREE, &dir_arg);
        return;
    }

    ESP_LOGI(TAG, "Read File: %s", full_path);
    if (fs_strcmp_ext(dir->d_name, ".json")) return;

    // Validate language:

    char* buffer = fs_read_file(full_path);
    if (buffer == NULL) return;

    cJSON* root = cJSON_Parse(buffer);
    free(buffer);

    if (cJSON_IsObject(root)) {
        cJSON* lang = cJSON_GetObjectItem(root, "language");

        if (lang != NULL) {
            char* lang_file = NULL;
            // Dynamically allocate so we can add it as a menu arg, but also, skip the SD root to
            // save space.
            asiprintf(&lang_file, "%s", full_path + strlen(fs_sd_root()) + 1);

            if (lang_file != NULL) {
                ESP_LOGI(TAG, "Adding language: %s", lang->valuestring);

                ui_menu_item_t* item = ui_menu_add_item(
                    arg->menu, lang->valuestring, on_language_select, (void*)lang_file
                );

                item->freer = free;
            }
        }
    }

    cJSON_Delete(root);
}

static void on_language_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    ui_menu_add_item(m, "English", on_language_select, NULL);

    struct dir_result_arg dir_arg = { .menu = m, .path = fs_sd_root(), .depth = 0 };

    fs_read_dir(fs_sd_root(), _dir_result, FS_READ_NO_HIDDEN | FS_READ_NO_FREE, &dir_arg);
}

DYNAMIC_MENU(UI_LANGUAGE_MENU, "Language", on_language_open);