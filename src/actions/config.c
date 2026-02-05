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

static int
host_set_plugin_config(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t count) {
    cJSON* config = mta_plugin_get_config(plugin);
    const char* plugin_name = mta_plugin_get_name(plugin);

    if (count < 2 || !config || !plugin_name) return -1;

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

    char* config_path = NULL;
    asiprintf(&config_path, "%s/plugincfg/%s.json", eom_hal_get_sd_mount_point(), plugin_name);

    if (config_path) {
        FILE* f = fopen(config_path, "w");
        if (f) {
            char* json_str = cJSON_Print(config);
            if (json_str) {
                fputs(json_str, f);
                free(json_str);
            }
            fclose(f);
        }
        free(config_path);
    }

    return 0;
}

void action_config_init(void) {
    mta_register_system_function("getSystemConfig", host_get_system_config, PERM_SYSCFG_READ);
    mta_register_system_function("setSystemConfig", host_set_system_config, PERM_SYSCFG_WRITE);
    mta_register_system_function("getPluginConfig", host_get_plugin_config, NULL);
    mta_register_system_function("setPluginConfig", host_set_plugin_config, NULL);
}
