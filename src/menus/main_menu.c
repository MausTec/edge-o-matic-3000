#include "config.h"
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

static void on_info(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE menu_arg) {
    const char* serial = eom_hal_get_device_serial_const();

    if (EOM_VERSION_VALID) {
        // Display structured fields from the parsed semver.
        // "Ver" shows MAJOR.MINOR.PATCH[-prerelease] without build metadata.
        // "Build" shows the build metadata (commit SHA + branch) when present.
        char ver_core[60] = { 0 };
        if (EOM_PARSED_VERSION.prerelease != NULL) {
            snprintf(
                ver_core,
                sizeof(ver_core),
                "%d.%d.%d-%s",
                EOM_PARSED_VERSION.major,
                EOM_PARSED_VERSION.minor,
                EOM_PARSED_VERSION.patch,
                EOM_PARSED_VERSION.prerelease
            );
        } else {
            snprintf(
                ver_core,
                sizeof(ver_core),
                "%d.%d.%d",
                EOM_PARSED_VERSION.major,
                EOM_PARSED_VERSION.minor,
                EOM_PARSED_VERSION.patch
            );
        }

        ui_toast("S/N: %s\nVer: %s", serial, ver_core);

        if (EOM_PARSED_VERSION.metadata != NULL) {
            ui_toast_append("Build: %s", EOM_PARSED_VERSION.metadata);
        }
    } else {
        // Fallback: version string is not valid SemVer (e.g. a non-standard internal build).
        ui_toast("S/N: %s\nVer: %s", serial, EOM_VERSION);
    }
}

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
#ifdef EOM_WASM_APPS_ENABLED
    ui_menu_add_menu(m, &APPLICATIONS_MENU);
#endif
    ui_menu_add_menu(m, &EDGING_SETTINGS_MENU);
    ui_menu_add_menu(m, &ORGASM_SETTINGS_MENU);
    ui_menu_add_menu(m, &UI_SETTINGS_MENU);
#ifdef EOM_BETA
    ui_menu_add_menu(m, &USER_PROFILES_MENU);
#endif
    ui_menu_add_menu(m, &PLUGIN_SETTINGS_MENU);
    ui_menu_add_menu(m, &NETWORKING_MENU);
    ui_menu_add_menu(m, &UPDATE_MENU);
    ui_menu_add_item(m, _("System Info"), on_info, NULL);
}

DYNAMIC_MENU(MAIN_MENU, "Main Menu", on_open);