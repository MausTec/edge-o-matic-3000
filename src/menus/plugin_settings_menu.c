#include "menus/index.h"
#include "system/action_manager.h"
#include "ui/menu.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "util/i18n.h"

static void on_plugin_select(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE arg) {
    mta_plugin_t* plugin = (mta_plugin_t*)arg;
    if (!plugin) return;

    // TODO: Open plugin config submenu
}

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    size_t count = action_manager_get_plugin_count();

    if (count == 0) {
        ui_toast("%s", _("No plugins loaded"));
        ui_close_menu();
        return;
    }

    for (size_t i = 0; i < count; i++) {
        mta_plugin_t* plugin = action_manager_get_plugin(i);
        const char* name = mta_plugin_get_name(plugin);
        if (name) {
            ui_menu_add_item(m, name, on_plugin_select, plugin);
        }
    }
}

DYNAMIC_MENU(PLUGIN_SETTINGS_MENU, "Plugins", on_open);
