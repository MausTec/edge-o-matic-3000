#include "esp_log.h"
#include "maus_bus.h"
#include "ui/menu.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "util/i18n.h"
#include <string.h>

static const char* TAG = "accessory_port_menu";

static void on_pair_start(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE arg) {
    ui_toast_blocking("%s", _("Connecting..."));

    maus_bus_address_t address = (maus_bus_address_t)item->arg;
    maus_bus_device_t* device = maus_bus_get_scan_item_by_address(address);

    if (device == NULL) {
        ui_toast("%s", _("Device disconnected!"));
        return;
    }

    maus_bus_err_t err = maus_bus_register_device(address);

    if (err == MAUS_BUS_NOT_SUPPORTED) {
        ui_toast("%s", _("Unsupported device."));
        return;
    }

    if (err != MAUS_BUS_OK) {
        ESP_LOGE(TAG, "Failed to regsister device: %d", err);
        ui_toast("Error: %d", err);
        return;
    }

    ui_toast("Connected to: %s", device->product_name);
    ui_close_menu();
}

static void on_scan_result(maus_bus_device_t* device, maus_bus_address_t address, void* arg) {
    const ui_menu_t* m = (const ui_menu_t*)arg;

    char path[20];
    maus_bus_addr2str(path, 20, address);
    ESP_LOGD(TAG, "Found device: %s", path);

    ui_menu_item_t* item = ui_menu_add_item(m, device->product_name, on_pair_start, address);
}

static void on_scan_start(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE arg) {
    maus_bus_free_device_scan();
    ui_menu_clear_at(m, 1);
    ui_toast_blocking("%s", _("Scanning for devices..."));
    maus_bus_scan_bus_full(on_scan_result, (void*)m);
    ui_toast_clear();
}

static const ui_menu_item_t scan_start_item = {
    StaticMenuItemValues("Start Scan...", on_scan_start, NULL),
    .select_str = "START",
};

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    ui_menu_add_item_at(m, 0, &scan_start_item);
}

static void on_close(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    maus_bus_free_device_scan();
}

DYNAMIC_MENU_BASE(ACCESSORY_PORT_MENU, "Accessory Port", on_open, .on_close = on_close);