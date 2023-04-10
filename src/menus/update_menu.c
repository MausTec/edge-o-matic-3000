#include "eom-hal.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "update_manager.h"
#include "util/i18n.h"
#include <string.h>

static const char* TAG = "update_menu";

static void on_update_progress(um_progress_event_t evt, int current, int total, void* arg) {
    ui_menu_t* m = (ui_menu_t*)arg;

    switch (evt) {
    case UM_PROG_PREFLIGHT_DONE: {
        ui_toast_blocking("%s", _("Installing..."));
        break;
    }

    case UM_PROG_PAYLOAD_DATA: {
        ui_toast_set_progress(current, total);
        break;
    }

    case UM_PROG_PAYLOAD_DONE: {
        ui_toast_blocking("%s", _("Verifying..."));
        break;
    }

    default: return;
    }
}

static void
on_network_update(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE arg) {
    ui_toast_blocking("%s", _("Preparing..."));
    esp_err_t err = update_manager_update_from_web(on_update_progress, (void*)m);

    if (err != ESP_OK) {
        ui_toast(_("Failed: %s"), esp_err_to_name(err));
    } else {
        ui_toast_blocking("%s", _("Update successful.\nPlease restart."));
        while (1)
            vTaskDelay(1);
    }
}

static void on_local_update(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE arg) {
    ui_toast_blocking("%s", _("Preparing..."));
    esp_err_t err =
        update_manager_update_from_storage((const char*)item->arg, on_update_progress, (void*)m);

    if (err != ESP_OK) {
        ui_toast(_("Failed: %s"), esp_err_to_name(err));
    } else {
        ui_toast_blocking("%s", _("Update successful.\nPlease restart."));
        while (1)
            vTaskDelay(1);
    }
}

static void on_local_update_found(const um_update_info_t* info, void* arg) {
    ui_menu_t* m = (ui_menu_t*)arg;
    char label[UI_MENU_TITLE_MAX] = "SD: ";
    strlcat(label, info->version, UI_MENU_TITLE_MAX);

    ui_menu_item_t* item = ui_menu_add_item(m, label, on_local_update, NULL);
    asiprintf((char**)&item->arg, "%s", info->path);
    item->freer = free;
}

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    um_update_status_t online_status = UM_UPDATE_NOT_CHECKED;
    um_update_status_t local_status = UM_UPDATE_NOT_CHECKED;

    ui_toast_blocking("%s", _("Checking for updates..."));
    online_status = update_manager_check_online_updates();

    if (online_status == UM_UPDATE_AVAILABLE) {
        ui_menu_add_item(m, _("Update via Internet"), on_network_update, NULL);
    }

    local_status = update_manager_list_local_updates(on_local_update_found, (void*)m);

    if (online_status != UM_UPDATE_AVAILABLE && local_status != UM_UPDATE_AVAILABLE) {
        ui_toast("%s", _("No updates found."));
        ui_close_menu();
        return;
    }

    ui_toast_clear();
}

DYNAMIC_MENU(UPDATE_MENU, "Update", on_open);