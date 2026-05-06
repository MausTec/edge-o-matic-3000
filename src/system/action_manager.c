#include "system/action_manager.h"
#include "../../lib/mt-actions/include/mt_actions.h"
#include "actions/index.h"
#include "cJSON.h"
#include "eom-hal.h"
#include "system/event_manager.h"
#include "ui/toast.h"
#include "util/list.h"
#include "util/strcase.h"
#include <dirent.h>
#include <esp_log.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static const char* TAG = "system:action_manager";

typedef struct {
    mta_plugin_t* plugin;
    mta_scope_t* scope; // NULL for ble_driver plugins (scope is per-instance)
} plugin_entry_t;

static list_t _plugins = LIST_DEFAULT();

#define PLUGIN_DIR "plugins"
#define PLUGINCFG_DIR "plugincfg"

void action_manager_load_all_plugins(void) {
    DIR* d;
    struct dirent* dir;
    char* path = NULL;
    asiprintf(&path, "%s/%s", eom_hal_get_sd_mount_point(), PLUGIN_DIR);
    if (path == NULL) return;

    d = opendir(path);

    if (d) {
        while ((dir = readdir(d)) != NULL) {
            // Flat file: /plugins/foo.json
            if (dir->d_type == DT_REG && !strcmp(dir->d_name + strlen(dir->d_name) - 5, ".json") &&
                dir->d_name[0] != '.') {
                char* file = NULL;
                asiprintf(&file, "%s/%s", path, dir->d_name);
                if (file == NULL) break;

                action_manager_load_plugin(file);
                free(file);
            }

            // Named subfolder: /plugins/foo/plugin.json
            if (dir->d_type == DT_DIR && dir->d_name[0] != '.') {
                char* file = NULL;
                asiprintf(&file, "%s/%s/plugin.json", path, dir->d_name);
                if (file != NULL) {
                    FILE* probe = fopen(file, "r");
                    if (probe != NULL) {
                        fclose(probe);
                        action_manager_load_plugin(file);
                    }
                    free(file);
                }
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

    // Load user config from /plugincfg/<name>.json
    const char* plugin_name = mta_plugin_get_name(plugin);
    cJSON* user_config = NULL;

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
                        user_config = cJSON_ParseWithLength(cfg_buffer, cfg_size);
                    }

                    free(cfg_buffer);
                }

                fclose(cfg_file);
            }

            free(config_path);
        }
    }

    mta_plugin_set_config(plugin, user_config);
    if (user_config) cJSON_Delete(user_config);

    // Reject duplicate plugin names (e.g. both flat file and subfolder present)
    if (plugin_name && action_manager_find_plugin(plugin_name)) {
        ESP_LOGW(
            TAG, "Plugin '%s' already loaded — skipping duplicate from: %s", plugin_name, path
        );
        mta_plugin_free(plugin);
        goto cleanup;
    }

    plugin_entry_t* entry = malloc(sizeof(plugin_entry_t));

    if (!entry) {
        mta_plugin_free(plugin);
        goto cleanup;
    }

    entry->plugin = plugin;
    entry->scope = NULL;

    const char* plugin_type = mta_plugin_get_type(plugin);

    if (!plugin_type || strcmp(plugin_type, "ble_driver") != 0) {
        entry->scope = mta_scope_create();
        if (entry->scope) {
            mta_init_scope_from_plugin_vars(plugin, entry->scope);
        }
    }

    list_add(&_plugins, (void*)entry);

cleanup:
    cJSON_Delete(plugin_json);
    free(buffer);
    fclose(f);
}

void action_manager_dispatch_event(const char* event, int arg) {
    if (strncmp("EVT_", event, 4)) return;

    size_t evt_name_len = str_to_snake_case(NULL, 0, event + 4);
    if (evt_name_len == -1) return;

    char* evt_name = (char*)malloc(evt_name_len + 1);
    if (evt_name == NULL) return;

    str_to_snake_case(evt_name, evt_name_len + 1, event + 4);

    plugin_entry_t* entry = NULL;

    list_foreach(_plugins, entry) {
        const char* type = mta_plugin_get_type(entry->plugin);
        if (type && strcmp(type, "ble_driver") == 0) continue;

        mta_event_invoke_with_scope(entry->plugin, evt_name, arg, entry->scope, NULL);
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

static void action_manager_error_reporter(
    mta_plugin_t* plugin, const char* msg, int kind_int, void* user_data
) {
    (void)user_data;
    const char* plugin_name = plugin ? mta_plugin_get_name(plugin) : NULL;
    const char* kind_str = mta_runtime_error_kind_str((mta_runtime_error_kind_t)kind_int);

    ESP_LOGE(TAG, "[%s] %s: %s", plugin_name ? plugin_name : "?", kind_str, msg);
    ui_toast("Plugin error (%s)\n%s", plugin_name ? plugin_name : "?", msg);
}

void action_manager_init(void) {
    ESP_LOGI(TAG, "Initializing action manager...");

    mta_init_builtins();
    actions_register_all();

    mta_set_error_reporter(action_manager_error_reporter, NULL);
    mta_set_config_save_handler(action_manager_save_plugin_config, NULL);

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
        if (i == index) {
            plugin_entry_t* entry = (plugin_entry_t*)node->data;
            return entry ? entry->plugin : NULL;
        }

        i++;
        node = node->next;
    }

    return NULL;
}

mta_plugin_t* action_manager_find_plugin(const char* name) {
    if (!name) return NULL;
    plugin_entry_t* entry = NULL;

    list_foreach(_plugins, entry) {
        const char* plugin_name = mta_plugin_get_name(entry->plugin);
        if (plugin_name && strcmp(plugin_name, name) == 0) return entry->plugin;
    }

    return NULL;
}

bool action_manager_save_plugin_config(mta_plugin_t* plugin, void* user_data) {
    if (!plugin) return false;

    const char* plugin_name = mta_plugin_get_name(plugin);
    if (!plugin_name) return false;

    cJSON* config_json = mta_plugin_serialize_config(plugin);
    if (!config_json) return false;

    // Ensure the plugincfg directory exists
    char* dir_path = NULL;
    asiprintf(&dir_path, "%s/%s", eom_hal_get_sd_mount_point(), PLUGINCFG_DIR);

    if (dir_path) {
        mkdir(dir_path, 0755);
        free(dir_path);
    }

    char* config_path = NULL;
    asiprintf(
        &config_path, "%s/%s/%s.json", eom_hal_get_sd_mount_point(), PLUGINCFG_DIR, plugin_name
    );

    bool success = false;
    if (config_path) {
        FILE* f = fopen(config_path, "w");

        if (f) {
            char* json_str = cJSON_PrintUnformatted(config_json);
            if (json_str) {
                fputs(json_str, f);
                success = true;
                free(json_str);
            }
            fclose(f);
        }
        
        free(config_path);
    }

    cJSON_Delete(config_json);
    return success;
}