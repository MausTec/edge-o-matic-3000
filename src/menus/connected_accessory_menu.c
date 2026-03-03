#include "bluetooth_manager.h"
#include "drivers/plugin_driver.h"
#include "maus_bus.h"
#include "ui/menu.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "util/i18n.h"

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg);

static void on_cu_device(
    maus_bus_driver_t* driver, maus_bus_device_t* device, maus_bus_address_t address, void* arg
) {
    if (device == NULL) return;
    ui_menu_t* m = (ui_menu_t*)arg;
    assert(m != NULL);

    ui_menu_add_item(m, device->product_name, NULL, NULL);
}

static void on_bt_disconnect(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE arg) {
    peer_t* peer = (peer_t*)item->arg;
    if (!peer) return;

    ui_toast_blocking("%s", _("Disconnecting..."));
    bluetooth_manager_disconnect(peer);
    // GAP disconnect handler frees the peer — don't touch it after this.
    // Small delay to let the async disconnect complete before refreshing.
    vTaskDelay(pdMS_TO_TICKS(100));
    ui_menu_clear(m);
    on_open(m, arg);
    ui_toast_clear();
}

static void on_bt_device(peer_t* device, void* arg) {
    if (device == NULL) return;
    ui_menu_t* m = (ui_menu_t*)arg;
    assert(m != NULL);

    ui_menu_item_t* item = ui_menu_add_item(m, device->name, on_bt_disconnect, (void*)device);
    if (item) {
        strncpy(item->select_str, "DISC", sizeof(item->select_str) - 1);
    }
}

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    ui_toast_blocking("%s", _("Loading..."));
    maus_bus_enumerate_devices(on_cu_device, (void*)m);
    plugin_driver_enumerate(on_bt_device, (void*)m);
    ui_toast_clear();
}

DYNAMIC_MENU(CONNECTED_ACCESSORIES_MENU, "Connected Accessories", on_open);