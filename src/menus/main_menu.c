#include "esp_log.h"
#include "menus/index.h"
#include "ui/menu.h"
#include "ui/toast.h"
#include "util/i18n.h"
#include "version.h"
#include <string.h>

static const char* TAG = "main_menu";

// todo: probably we can statically allocate this whole menu but right now let's just do a dynamic
// allocator

static void on_info(const ui_menu_t* m, const ui_menu_item_t* item, const void* menu_arg) {
    const char* serial = eom_hal_get_device_serial_const();
    const char* sha = strchr(VERSION, '-');
    const char* tag = strchr(VERSION, '+');

    ui_toast("S/N: %s\nVer: %.*s", serial, sha == NULL ? TOAST_MAX : sha - VERSION, VERSION);

    if (sha != NULL) {
        ui_toast_append("SHA: %.*s", tag == NULL ? TOAST_MAX : tag - sha - 1, sha + 1);
    }

    if (tag != NULL) {
        ui_toast_append("Tag: %s", tag + 1);
    }
}

static void on_open(const ui_menu_t* m, const void* arg) {
#ifdef EOM_BETA
    ui_menu_add_menu(m, &APPLICATIONS_MENU);
#endif
    ui_menu_add_menu(m, &EDGING_SETTINGS_MENU);
    ui_menu_add_menu(m, &ORGASM_SETTINGS_MENU);
    ui_menu_add_menu(m, &UI_SETTINGS_MENU);
    ui_menu_add_menu(m, &USER_PROFILES_MENU);
    ui_menu_add_menu(m, &NETWORKING_MENU);
    ui_menu_add_menu(m, &UPDATE_MENU);
    ui_menu_add_item(m, _("System Info"), on_info, NULL);
}

DYNAMIC_MENU(MAIN_MENU, "Main Menu", on_open);