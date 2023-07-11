#include "bluetooth_driver.h"
#include "maus_bus.h"
#include "ui/menu.h"
#include "ui/toast.h"
#include "util/i18n.h"

static void on_cu_device(
    maus_bus_driver_t* driver, maus_bus_device_t* device, maus_bus_address_t address, void* arg
) {
    if (device == NULL) return;
    ui_menu_t* m = (ui_menu_t*)arg;
    assert(m != NULL);

    ui_menu_add_item(m, device->product_name, NULL, NULL);
}

static void on_bt_device(peer_t* device, void* arg) {
    if (device == NULL) return;
    ui_menu_t* m = (ui_menu_t*)arg;
    assert(m != NULL);

    ui_menu_add_item(m, device->name, NULL, NULL);
}

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    ui_toast_blocking("%s", _("Loading..."));
    maus_bus_enumerate_devices(on_cu_device, (void*)m);
    bluetooth_driver_enumerate_devices(on_bt_device, (void*)m);
    ui_toast_clear();
}

DYNAMIC_MENU(CONNECTED_ACCESSORIES_MENU, "Connected Accessories", on_open);