#include "bluetooth_driver.h"
#include "bluetooth_manager.h"
#include "esp_log.h"
#include "menus/common.h"
#include "ui/menu.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "util/i18n.h"
#include <stdbool.h>
#include <string.h>

static bool _scanning = false;
static const char* TAG = "bluetooth_scan_menu";

static void update_scan_item(const ui_menu_t* m);

static void on_pair_start(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE arg) {
    peer_t* peer = (peer_t*)item->arg;
    if (m == NULL || item == NULL || peer == NULL) return;

    int rc;
    ui_toast_blocking("%s", _("Connecting..."));

    _scanning = false;
    update_scan_item(m);

    rc = bluetooth_manager_connect(peer);

    if (rc == BLE_HS_EDONE) {
        ui_toast("%s", _("Already connected."));
    } else if (rc != 0) {
        ESP_LOGE(TAG, "Failed to connect: %d", rc);
        ui_toast(_("Failed to connect: %d"), rc);
    } else {
        peer->driver = bluetooth_driver_find_for_peer(peer);

        if (peer->driver == NULL) {
            bluetooth_manager_disconnect(peer);
            ui_toast("%s", _("Unable to find suitable driver."));
            return;
        }

        bluetooth_driver_register_peer(peer);

        ui_toast("%s", _("Connected!"));
        ui_close_menu();
        // TODO: open bt devices menu
    }
}

static void on_scan_result(peer_t* peer, void* arg) {
    const ui_menu_t* m = (const ui_menu_t*)arg;

    peer_t* peer_cpy = (peer_t*)malloc(sizeof(peer_t));
    if (peer_cpy == NULL) return;

    memcpy(peer_cpy, peer, sizeof(peer_t));

    ui_menu_item_t* item = ui_menu_add_item(m, peer->name, on_pair_start, peer_cpy);
    if (item == NULL) return;
    item->freer = free;
}

static void on_scan_start(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE arg) {
    _scanning = true;
    ui_menu_clear(m);
    update_scan_item(m);
    bluetooth_manager_start_scan(on_scan_result, (void*)m);
}

static void on_scan_stop(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE arg) {
    _scanning = false;
    update_scan_item(m);
    bluetooth_manager_stop_scan();
}

static const ui_menu_item_t scan_start_item = {
    StaticMenuItemValues("Start Scan...", on_scan_start, NULL),
    .select_str = "START",
};

static const ui_menu_item_t scan_stop_item = {
    StaticMenuItemValues("Scanning for Devices...", on_scan_stop, NULL),
    .select_str = "STOP",
};

static void update_scan_item(const ui_menu_t* m) {
    if (!_scanning) {
        ui_menu_replace_item_at(m, 0, &scan_start_item);
    } else {
        ui_menu_replace_item_at(m, 0, &scan_stop_item);
    }
}

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    _scanning = false;
    ui_menu_add_node(m, &scan_start_item, NULL);
    bluetooth_manager_start_advertising();
}

static void on_close(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    bluetooth_manager_stop_scan();
    bluetooth_manager_stop_advertising();
}

DYNAMIC_MENU_BASE(BLUETOOTH_SCAN_MENU, "Scan for Devices", on_open, .on_close = on_close);