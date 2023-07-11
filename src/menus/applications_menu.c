#include "application.h"
#include "eom-hal.h"
#include "esp_log.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "util/i18n.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>

static const char* TAG = "applications_menu";

static void on_app_load(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE menu_arg) {
    if (item == NULL) return;
    const char* path = (const char*)item->arg;
    application_t* app;

    ui_toast_blocking(_("Loading app:\n%s"), path);
    app_err_t err = application_load(path, &app);

    if (err == APP_OK) {
        ui_toast(_("Loaded"));
        application_start_background(app);

        if (err == APP_OK) {
            ESP_LOGI(TAG, "Application started and routine returned.");
            ui_close_all();
            return;
        }

        ui_toast(_("App Start Error!\nERR %d"), err);

    } else {
        ui_toast(_("App Load Error:\nERR %d"), err);
    }
}

static void populate_apps(const ui_menu_t* m) {
    DIR* d;
    struct dirent* dir;
    const char* path = eom_hal_get_sd_mount_point();
    d = opendir(path);

    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_DIR && !strcmp(dir->d_name + strlen(dir->d_name) - 4, ".mpk")) {
                char* file = (char*)malloc(strlen(dir->d_name) + strlen(path) + 2);
                if (file == NULL) break;
                sprintf(file, "%s/%s", path, dir->d_name);
                ui_menu_item_t* item = ui_menu_add_item(m, dir->d_name, on_app_load, file);
                item->freer = free;
            }
        }

        closedir(d);
    }
}

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    // ui_menu_add_page(PAGE_EDGING_STATS);

    if (eom_hal_get_sd_size_bytes() > 0) {
        // TODO: this won't actually render out in this routine.
        ui_toast_blocking(_("Reading SD card..."));
        populate_apps(m);
        ui_toast_clear();
    }
}

DYNAMIC_MENU(APPLICATIONS_MENU, "Applications", on_open);