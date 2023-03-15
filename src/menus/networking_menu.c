#include "config.h"
#include "menus/common.h"
#include "menus/index.h"
#include "ui/input.h"
#include "ui/menu.h"

const ui_input_toggle_t BLUETOOTH_ON_INPUT = {
    ToggleInputValues("Enable Bluetooth", &Config.bt_on, on_config_save),
};

const ui_input_toggle_t WIFI_ON_INPUT = {
    ToggleInputValues("Enable WiFi", &Config.wifi_on, on_config_save),
};

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    ui_menu_add_input(m, (ui_input_t*)&BLUETOOTH_ON_INPUT);
    ui_menu_add_input(m, (ui_input_t*)&WIFI_ON_INPUT);
    ui_menu_add_menu(m, &ACCESSORY_PORT_MENU);
}

DYNAMIC_MENU(NETWORKING_MENU, "Networking", on_open);