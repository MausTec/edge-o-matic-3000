#include "bluetooth_driver.h"
#include "menus/common.h"
#include "string.h"
#include "ui/menu.h"
#include <stdbool.h>

static bool _scanning = false;

static void update_scan_item(const ui_menu_t* m);

static void on_pair_start(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE arg) {
}

static void on_scan_result(peer_t* peer, void* arg) {
    const ui_menu_t* m = (const ui_menu_t*)arg;
    ui_menu_add_item(m, peer->name, on_pair_start, peer);
}

static void on_scan_start(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE arg) {
    _scanning = true;
    update_scan_item(m);
    bluetooth_driver_start_scan(on_scan_result, (void*)m);
}

static void on_scan_stop(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE arg) {
    _scanning = false;
    update_scan_item(m);
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
        ui_menu_remove_item_at(m, 0);
        ui_menu_add_item_at(m, 0, &scan_start_item);
    } else {
        ui_menu_remove_item_at(m, 0);
        ui_menu_add_item_at(m, 0, &scan_stop_item);
    }
}

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    _scanning = false;
    ui_menu_add_node(m, &scan_start_item, NULL);
}

static void on_close(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
}

DYNAMIC_MENU_BASE(
    BLUETOOTH_SCAN_MENU,
    "Scan for Devices",
    on_open,
    .on_close = on_close,
    .flags.translate_title = 0,
);