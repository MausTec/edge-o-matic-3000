#include "menus/index.h"
#include "system/action_manager.h"
#include "ui/input_helpers.h"
#include "ui/menu.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "util/i18n.h"
#include <stdlib.h>
#include <string.h>

static void on_plugin_config_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg);
DYNAMIC_MENU(PLUGIN_CONFIG_MENU, "", on_plugin_config_open);

// Static storage for plugin config input, populated on-demand in the select callback.
static ui_input_any_t _plugin_config_input;
static ui_input_value_any_t _plugin_config_value;

// Input field definition for menu item context pointer
typedef struct {
    mta_plugin_t* plugin;
    char field_name[64];
    mta_config_type_t type;
    int min;
    int max;
    bool has_min;
    bool has_max;
} config_field_context_t;

/**
 * @brief Generic save callback for config fields, updates plugin state and saves to SD card.
 *
 * @param value
 * @param arg
 */
static void on_config_save(int value, int final, UI_INPUT_ARG_TYPE arg) {
    if (!final) return;

    config_field_context_t* ctx = (config_field_context_t*)arg;
    if (!ctx || !ctx->plugin) return;

    mta_config_value_t config_val;
    config_val.type = ctx->type;

    switch (ctx->type) {
    case MTA_CONFIG_TYPE_BOOL: config_val.value.bool_value = (value != 0); break;
    case MTA_CONFIG_TYPE_INT: config_val.value.int_value = value; break;
    default: return;
    }

    if (!mta_plugin_set_config_value(ctx->plugin, ctx->field_name, &config_val)) {
        ui_toast(_("Update failed"));
        return;
    }

    if (!action_manager_save_plugin_config(ctx->plugin)) {
        ui_toast(_("Save failed"));
    }
}

/**
 * @brief Plugin setting select callback, creates an input field based on the setting type.
 *
 * @param m
 * @param item
 * @param menu_arg
 */
static void
on_config_field_select(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE menu_arg) {
    config_field_context_t* ctx = (config_field_context_t*)item->arg;
    if (!ctx || !ctx->plugin) return;

    mta_config_value_t current_value;
    if (!mta_plugin_get_config_value(ctx->plugin, ctx->field_name, &current_value)) {
        return;
    }

    // Common fields
    _plugin_config_input.base.title = item->label;
    _plugin_config_input.base.help = NULL; // TODO: Store in ctx if needed
    _plugin_config_input.base.flags.translate_title = 0;

    // Set type-specific fields
    switch (ctx->type) {
    case MTA_CONFIG_TYPE_BOOL:
        _plugin_config_input.base.type = INPUT_TYPE_TOGGLE;
        _plugin_config_value.bool_val = current_value.value.bool_value;

        _plugin_config_input.toggle.value = &_plugin_config_value.bool_val;
        _plugin_config_input.toggle.on_save = on_config_save;
        break;

    case MTA_CONFIG_TYPE_INT:
        _plugin_config_input.base.type = INPUT_TYPE_NUMERIC;
        _plugin_config_value.int_val = current_value.value.int_value;

        _plugin_config_input.numeric.unit = "";
        _plugin_config_input.numeric.value = &_plugin_config_value.int_val;
        _plugin_config_input.numeric.min = ctx->has_min ? ctx->min : INT_MIN;
        _plugin_config_input.numeric.max = ctx->has_max ? ctx->max : INT_MAX;
        _plugin_config_input.numeric.step = 1;
        _plugin_config_input.numeric.on_save = on_config_save;
        _plugin_config_input.numeric.chart_getter = NULL;
        break;

    case MTA_CONFIG_TYPE_STRING:
        // TODO: Text input not yet implemented
        return;
    }

    ui_open_input(&_plugin_config_input.base, ctx);
}

// Context passed to the MTA iterator for generating menu items.
typedef struct {
    const ui_menu_t* menu;
    mta_plugin_t* plugin;
} populate_context_t;

/**
 * Called by the MTA iterator for each config field to populate the menu.
 */
static void populate_config_field(const mta_config_field_info_t* field_info, void* user_data) {
    populate_context_t* ctx = (populate_context_t*)user_data;
    const char* label = field_info->label ? field_info->label : field_info->field_name;

    // Allocate config item context
    config_field_context_t* field_ctx =
        (config_field_context_t*)malloc(sizeof(config_field_context_t));
    if (!field_ctx) return;

    field_ctx->plugin = ctx->plugin;
    strncpy(field_ctx->field_name, field_info->field_name, sizeof(field_ctx->field_name) - 1);
    field_ctx->field_name[sizeof(field_ctx->field_name) - 1] = '\0';
    field_ctx->type = field_info->type;
    field_ctx->min = field_info->min;
    field_ctx->max = field_info->max;
    field_ctx->has_min = field_info->has_min;
    field_ctx->has_max = field_info->has_max;

    // Add to menu
    ui_menu_item_t* item = ui_menu_add_item(ctx->menu, label, on_config_field_select, field_ctx);
    if (item) {
        item->freer = free; // Auto-cleanup context
        strncpy(item->select_str, _("EDIT"), UI_BUTTON_STR_MAX);
    }
}

static void on_plugin_config_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    mta_plugin_t* plugin = (mta_plugin_t*)arg;
    if (!plugin) return;

    const char* plugin_name = mta_plugin_get_name(plugin);
    if (plugin_name) {
        snprintf((char*)m->title, UI_MENU_TITLE_MAX, "%s Config", plugin_name);
    }

    populate_context_t ctx = { .menu = m, .plugin = plugin };
    mta_plugin_iterate_config_fields(plugin, populate_config_field, &ctx);
}

static void on_plugin_select(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE arg) {
    mta_plugin_t* plugin = (mta_plugin_t*)arg;
    if (!plugin) return;

    if (!mta_plugin_has_config_schema(plugin)) {
        ui_toast("%s", _("No config available"));
        return;
    }

    ui_open_menu(&PLUGIN_CONFIG_MENU, plugin);
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

DYNAMIC_MENU(PLUGIN_SETTINGS_MENU, "Plugin Settings", on_open);
