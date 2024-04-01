#include "system/action_manager.h"
#include "cJSON.h"
#include "eom-hal.h"
#include "mt_actions.h"
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
#define DRIVERCFG_DIR "drivercfg"

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
    mta_load(&plugin, plugin_json);

    if (plugin == NULL) {
        goto cleanup;
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
    event_manager_register_handler(EVT_ALL, action_manager_event_handler, NULL);
}