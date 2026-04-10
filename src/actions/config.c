#include "config.h"
#include "actions/index.h"
#include "eom-hal.h"
#include "mt_actions.h"
#include "system/action_manager.h"
#include <cJSON.h>
#include <stdio.h>
#include <string.h>

#define PERM_SYSCFG_READ "syscfg:read"
#define PERM_SYSCFG_WRITE "syscfg:write"

/**
 * Read a system configuration value by key.
 *
 * @plugin getSystemConfig
 * @module config
 * @arg key:string Configuration key path
 * @returns string Configuration value as string
 */
static int
host_get_system_config(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t count) {
    if (count < 1) return -1;

    const char* key = mta_arg_get_string(plugin, scope, args, 0);
    if (!key) return -1;

    char buffer[256];
    if (get_config_value(key, buffer, sizeof(buffer))) {
        mta_return_string(plugin, scope, buffer);
        return 0;
    }

    return -1;
}

/**
 * Write a system configuration value by key.
 *
 * @plugin setSystemConfig
 * @module config
 * @arg key:string Configuration key path
 * @arg value:string New value to set
 * @returns int 1 if reboot required, 0 otherwise
 */
static int
host_set_system_config(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t count) {
    if (count < 2) return -1;

    const char* key = mta_arg_get_string(plugin, scope, args, 0);
    const char* value = mta_arg_get_string(plugin, scope, args, 1);
    if (!key || !value) return -1;

    bool require_reboot = false;
    if (set_config_value(key, value, &require_reboot)) {
        config_enqueue_save(0);
        mta_return_int(plugin, scope, require_reboot ? 1 : 0);
        return 0;
    }

    return -1;
}

/**
 * Read a value from the calling plugin's own configuration.
 *
 * @plugin getPluginConfig
 * @module config
 * @arg key:string Configuration key name
 * @arg default:int Default value if key not found (optional)
 * @returns int Configuration value (type depends on stored value)
 */
static int
host_get_plugin_config(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t count) {
    if (count < 1) return -1;

    const char* key = mta_arg_get_string(plugin, scope, args, 0);
    cJSON* config = mta_plugin_get_config(plugin);
    if (!key || !config) return -1;

    cJSON* value = cJSON_GetObjectItem(config, key);

    if (value) {
        if (cJSON_IsNumber(value)) {
            mta_return_int(plugin, scope, value->valueint);
            return 0;
        } else if (cJSON_IsString(value)) {
            mta_return_string(plugin, scope, value->valuestring);
            return 0;
        }
    }

    if (count >= 2) {
        int default_val = mta_arg_get_int(plugin, scope, args, 1);
        mta_return_int(plugin, scope, default_val);
        return 0;
    }

    return -1;
}

/**
 * Write a value to the calling plugin's own configuration.
 *
 * @plugin setPluginConfig
 * @module config
 * @arg key:string Configuration key name
 * @arg value:int Value to set (int, float, or string)
 * @returns int 0 on success
 */
static int
host_set_plugin_config(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t count) {
    cJSON* config = mta_plugin_get_config(plugin);
    if (count < 2 || !config) return -1;

    const char* key = mta_arg_get_string(plugin, scope, args, 0);
    if (!key) return -1;

    cJSON_DeleteItemFromObject(config, key);

    switch (mta_arg_get_type(plugin, scope, args, 1)) {
    case MTA_ARG_STRING_REF: {
        const char* str_val = mta_arg_get_string(plugin, scope, args, 1);
        if (str_val) cJSON_AddStringToObject(config, key, str_val);
        break;
    }
    case MTA_ARG_FLOAT:
        cJSON_AddNumberToObject(config, key, mta_arg_get_float(plugin, scope, args, 1));
        break;
    default: cJSON_AddNumberToObject(config, key, mta_arg_get_int(plugin, scope, args, 1)); break;
    }

    // Delegate to action_manager for persistence
    if (!action_manager_save_plugin_config(plugin)) {
        return -1;
    }

    return 0;
}

void action_config_init(void) {
    mta_register_system_function("getSystemConfig", host_get_system_config, PERM_SYSCFG_READ);
    mta_register_system_function("setSystemConfig", host_set_system_config, PERM_SYSCFG_WRITE);
    mta_register_system_function("getPluginConfig", host_get_plugin_config, NULL);
    mta_register_system_function("setPluginConfig", host_set_plugin_config, NULL);
}
