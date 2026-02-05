#include "system/action_manager.h"
#include "../../lib/mt-actions/include/mt_actions.h"
#include "actions/index.h"
#include "cJSON.h"
#include "config.h"
#include "eom-hal.h"
#include "system/event_manager.h"
#include "util/list.h"
#include "util/strcase.h"
#include <dirent.h>
#include <esp_log.h>
#include <stdio.h>
#include <string.h>

static const char* TAG = "system:action_manager";

static list_t _plugins = LIST_DEFAULT();

#define PLUGIN_DIR "plugins"
#define PLUGINCFG_DIR "plugincfg"

static int
host_get_system_config(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t count);
static int
host_set_system_config(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t count);
static int
host_get_plugin_config(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t count);
static int
host_set_plugin_config(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t count);

void action_manager_load_all_plugins(void) {
    DIR* d;
    struct dirent* dir;
    char* path = NULL;
    asiprintf(&path, "%s/%s", eom_hal_get_sd_mount_point(), PLUGIN_DIR);
    if (path == NULL) return;

    d = opendir(path);

    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG && !strcmp(dir->d_name + strlen(dir->d_name) - 5, ".json") &&
                dir->d_name[0] != '.') {
                char* file = NULL;
                asiprintf(&file, "%s/%s", path, dir->d_name);
                if (file == NULL) break;

                action_manager_load_plugin(file);
                free(file);
            }
        }

        closedir(d);
    }

    free(path);
}

void action_manager_load_plugin(const char* path) {
    ESP_LOGI(TAG, "Loading plugin: %s", path);
    long fsize;
    size_t result;
    char* buffer = NULL;
    cJSON* plugin_json = NULL;
    FILE* f = fopen(path, "r");

    if (!f) {
        return;
    }

    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    rewind(f);
    buffer = (char*)malloc(fsize + 1);

    if (!buffer) {
        goto cleanup;
    }

    result = fread(buffer, 1, fsize, f);
    buffer[fsize] = '\0';

    if (result != fsize) {
        goto cleanup;
    }

    plugin_json = cJSON_ParseWithLength(buffer, fsize);

    if (plugin_json == NULL) {
        goto cleanup;
    }

    mta_plugin_t* plugin = NULL;
    mta_load_plugin(&plugin, plugin_json);

    if (plugin == NULL) {
        goto cleanup;
    }

    // Load plugin name
    cJSON* name_json = cJSON_GetObjectItem(plugin_json, "name");
    if (name_json && cJSON_IsString(name_json)) {
        mta_plugin_set_name(plugin, name_json->valuestring);
    }

    // Load permissions array
    cJSON* permissions_json = cJSON_GetObjectItem(plugin_json, "permissions");
    if (permissions_json && cJSON_IsArray(permissions_json)) {
        mta_plugin_set_permissions(plugin, cJSON_Duplicate(permissions_json, 1));
    }

    // Load plugin config from /plugincfg/<name>.json
    const char* plugin_name = mta_plugin_get_name(plugin);
    if (plugin_name) {
        char* config_path = NULL;
        asiprintf(
            &config_path, "%s/%s/%s.json", eom_hal_get_sd_mount_point(), PLUGINCFG_DIR, plugin_name
        );

        if (config_path) {
            FILE* cfg_file = fopen(config_path, "r");
            if (cfg_file) {
                fseek(cfg_file, 0, SEEK_END);
                long cfg_size = ftell(cfg_file);
                rewind(cfg_file);

                char* cfg_buffer = (char*)malloc(cfg_size + 1);
                if (cfg_buffer) {
                    size_t cfg_read = fread(cfg_buffer, 1, cfg_size, cfg_file);
                    cfg_buffer[cfg_size] = '\0';

                    if (cfg_read == cfg_size) {
                        mta_plugin_set_config(plugin, cJSON_ParseWithLength(cfg_buffer, cfg_size));
                    }

                    free(cfg_buffer);
                }

                fclose(cfg_file);
            }

            free(config_path);
        }
    }

    if (!mta_plugin_get_config(plugin)) {
        mta_plugin_set_config(plugin, cJSON_CreateObject());
    }

    list_add(&_plugins, (void*)plugin);

cleanup:
    cJSON_Delete(plugin_json);
    free(buffer);
    fclose(f);
}

void action_manager_dispatch_event(const char* event, int arg) {
    mta_plugin_t* plugin = NULL;

    const char* evt_strip = strstr("EVT_", event);
    if (strncmp("EVT_", event, 4)) return;

    size_t evt_name_len = str_to_camel_case(NULL, 0, event + 4);
    if (evt_name_len == -1) return;

    char* evt_name = (char*)malloc(evt_name_len + 1);
    if (evt_name == NULL) return;

    str_to_camel_case(evt_name, evt_name_len + 1, event + 4);

    list_foreach(_plugins, plugin) {
        mta_event_invoke(plugin, evt_name, arg);
    }

    free(evt_name);
}

void action_manager_event_handler(
    const char* event,
    EVENT_HANDLER_ARG_TYPE event_arg_ptr,
    int event_arg_int,
    EVENT_HANDLER_ARG_TYPE handler_arg
) {
    action_manager_dispatch_event(event, event_arg_int);
}

void action_manager_init(void) {
    ESP_LOGI(TAG, "Initializing action manager...");

    mta_register_system_function_by_name("getSystemConfig", host_get_system_config);
    mta_register_system_function_by_name("setSystemConfig", host_set_system_config);
    mta_register_system_function_by_name("getPluginConfig", host_get_plugin_config);
    mta_register_system_function_by_name("setPluginConfig", host_set_plugin_config);

    event_manager_register_handler(EVT_ALL, action_manager_event_handler, NULL);

    ESP_LOGI(TAG, "Action manager initialized");
}

size_t action_manager_get_plugin_count(void) {
    size_t count = 0;
    list_node_t* node = _plugins._first;
    while (node) {
        count++;
        node = node->next;
    }
    return count;
}

mta_plugin_t* action_manager_get_plugin(size_t index) {
    size_t i = 0;
    list_node_t* node = _plugins._first;
    while (node) {
        if (i == index) return (mta_plugin_t*)node->data;
        i++;
        node = node->next;
    }
    return NULL;
}

mta_plugin_t* action_manager_find_plugin(const char* name) {
    if (!name) return NULL;
    mta_plugin_t* plugin = NULL;
    list_foreach(_plugins, plugin) {
        const char* plugin_name = mta_plugin_get_name(plugin);
        if (plugin_name && strcmp(plugin_name, name) == 0) return plugin;
    }
    return NULL;
}

// ==========================
// Host System Function Implementations
// ==========================

static int
host_get_system_config(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t count) {
    if (count < 1) return -1;

    const char* key = mta_arg_get_string(plugin, scope, args, 0);
    if (!key) return -1;

    // TODO: Permission check
    // if (!has_permission(plugin, "sysconfig:read")) return -1;

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

    // TODO: Permission check
    // if (!has_permission(plugin, "sysconfig:write")) return -1;

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

    // Return default if provided
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

    // Write back to disk
    char* config_path = NULL;
    asiprintf(
        &config_path, "%s/%s/%s.json", eom_hal_get_sd_mount_point(), PLUGINCFG_DIR, plugin_name
    );

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