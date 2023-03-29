#include "assets/config_help.h"
#include "bluetooth_manager.h"
#include "config.h"
#include "menus/common.h"
#include "menus/index.h"
#include "ui/input.h"
#include "ui/menu.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "util/i18n.h"
#include "wifi_manager.h"
#include <stdio.h>

static void on_wifi_state_save(int value, int final, UI_INPUT_ARG_TYPE arg) {
    if (!final) return;

    if (!value) {
        wifi_manager_deinit();
    } else {
        ui_toast_blocking("%s", _("Connecting..."));
        wifi_manager_init();
    }

    on_config_save(value, final, arg);
    ui_reenter_menu();
}

static void on_bluetooth_state_save(int value, int final, UI_INPUT_ARG_TYPE arg) {
    if (!final) return;

    if (!value) {
        bluetooth_manager_deinit();
    } else {
        bluetooth_manager_init();
    }

    on_config_save(value, final, arg);
    ui_reenter_menu();
}

static void
on_connection_status(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE arg) {
    if (wifi_manager_get_status() == WIFI_MANAGER_CONNECTED) {
        ui_toast("%s\nIP: %s", _("Wifi Connected"), wifi_manager_get_local_ip());
    } else {
        ui_toast("%s", _("WiFi Disconnected"));
    }

    if (Config.bt_on) {
        ui_toast_append("%s", _("Bluetooth On"));
    }
}

static void
on_default_wifi_connect(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE arg) {
    if (wifi_manager_get_status() == WIFI_MANAGER_CONNECTED) {
        wifi_manager_disconnect();
        ui_reenter_menu();
    } else {
        ui_toast_blocking(_("Connecting to %s..."), Config.wifi_ssid);
        esp_err_t err = wifi_manager_connect_to_ap(Config.wifi_ssid, Config.wifi_key);

        if (err == ESP_OK) {
            int ticks_wait = 0;

            while (wifi_manager_get_status() != WIFI_MANAGER_CONNECTED && ticks_wait < 100) {
                vTaskDelay(1);
                ticks_wait++;
                if (ticks_wait % 10 == 0) ui_toast_append(".");
            }

            if (wifi_manager_get_status() != WIFI_MANAGER_CONNECTED) {
                err = ticks_wait >= 100 ? ESP_ERR_WIFI_TIMEOUT : ESP_ERR_WIFI_NOT_CONNECT;
            }
        }

        if (err == ESP_OK) {
            ui_toast(_("Connected!"));
            ui_reenter_menu();
        } else {
            ui_toast(_("Failed to connect:\n%s"), esp_err_to_name(err));
        }
    }
}

const ui_input_toggle_t BLUETOOTH_ON_INPUT = {
    ToggleInputValues("Enable Bluetooth", &Config.bt_on, on_bluetooth_state_save),
    .input.help = BT_ON_HELP,
};

const ui_input_toggle_t WIFI_ON_INPUT = {
    ToggleInputValues("Enable WiFi", &Config.wifi_on, on_wifi_state_save),
    .input.help = WIFI_ON_HELP,
};

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    ui_menu_item_t* bt_item = ui_menu_add_input(m, (ui_input_t*)&BLUETOOTH_ON_INPUT);
    ui_menu_item_t* wifi_item = ui_menu_add_input(m, (ui_input_t*)&WIFI_ON_INPUT);

    if (Config.wifi_on) {
        if (!Config.force_bt_coex) {
            bt_item->on_select = NULL;
        }

        wifi_manager_status_t wifi_status = wifi_manager_get_status();

        if (Config.wifi_ssid[0] != '\0') {
            char label[UI_MENU_TITLE_MAX];

            if (wifi_status == WIFI_MANAGER_CONNECTED) {
                sniprintf(label, UI_MENU_TITLE_MAX, _("Disconnect from %s"), Config.wifi_ssid);
            } else {
                sniprintf(label, UI_MENU_TITLE_MAX, _("Connect to %s"), Config.wifi_ssid);
            }

            ui_menu_add_item(m, label, on_default_wifi_connect, NULL);
        }

        if (wifi_status != WIFI_MANAGER_CONNECTED) {
#ifdef EOM_BETA
            ui_menu_add_item(m, _("Scan for Networks"), NULL, NULL);
#endif
        } else {
#ifdef EOM_BETA
            ui_menu_add_item(m, _("Maus-Link Connect"), NULL, NULL);
#endif
        }
    }

    if (Config.bt_on) {
        if (!Config.force_bt_coex) {
            wifi_item->on_select = NULL;
        }

        ui_menu_add_menu(m, &BLUETOOTH_SCAN_MENU);
    }

    ui_menu_add_item(m, _("Connection Status"), on_connection_status, NULL);
    ui_menu_add_menu(m, &ACCESSORY_PORT_MENU);
}

DYNAMIC_MENU(NETWORKING_MENU, "Networking", on_open);