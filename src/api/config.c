#include "config.h"
#include "SDHelper.h"
#include "api/index.h"
#include "config_defs.h"
#include "eom-hal.h"
#include "system/websocket_handler.h"


static command_err_t cmd_config_list(cJSON* command, cJSON* response, websocket_client_t* client) {
    cJSON_AddStringToObject(response, "_filename", Config._filename);
    config_to_json(response, &Config);
    return CMD_OK;
}

static const websocket_command_t cmd_config_list_s = {
    .command = "configList",
    .func = &cmd_config_list,
};

static command_err_t cmd_config_load(cJSON* command, cJSON* response, websocket_client_t* client) {
    // if (argc == 0) {
    //     return CMD_ARG_ERR;
    // }

    // char path[PATH_MAX + 1] = { 0 };
    // SDHelper_getRelativePath(path, PATH_MAX, argv[0], console);

    // esp_err_t err = config_load_from_sd(path, &Config);
    // if (err != ESP_OK) {
    //     fprintf(console->out, "Failed to load config: %d\n", err);
    //     return CMD_FAIL;
    // }

    // fprintf(console->out, "Config loaded from: %s\n", Config._filename);
    return CMD_OK;
}

static const websocket_command_t cmd_config_load_s = {
    .command = "configLoad",
    .func = &cmd_config_load,
};

static command_err_t cmd_config_save(cJSON* command, cJSON* response, websocket_client_t* client) {
    // if (argc >= 2) {
    //     return CMD_ARG_ERR;
    // }

    // esp_err_t err;

    // if (argc == 0) {
    //     err = config_save_to_sd(Config._filename, &Config);
    // } else {
    //     char path[PATH_MAX + 1] = { 0 };
    //     SDHelper_getRelativePath(path, PATH_MAX, argv[0], console);
    //     err = config_save_to_sd(path, &Config);
    // }

    // if (err != ESP_OK) {
    //     fprintf(console->out, "Failed to store config: %d\n", err);
    //     return CMD_FAIL;
    // }

    // fprintf(console->out, "Config stored to: %s\n", Config._filename);
    return CMD_OK;
}

static const websocket_command_t cmd_config_save_s = {
    .command = "configSave",
    .func = &cmd_config_save,
};

static command_err_t cmd_config_set(cJSON* command, cJSON* response, websocket_client_t* client) {
    json_to_config_merge(command, &Config);
    config_enqueue_save(1);

    // Propagate changes to hardware:
    cJSON* sensor_sensitivity = cJSON_GetObjectItem(command, "sensor_sensitivity");
    if (sensor_sensitivity != NULL) {
        eom_hal_set_sensor_sensitivity(sensor_sensitivity->valueint);
    }

    return CMD_OK;
}

static const websocket_command_t cmd_config_set_s = {
    .command = "configSet",
    .func = &cmd_config_set,
};

void api_register_config(void) {
    websocket_register_command(&cmd_config_list_s);
    websocket_register_command(&cmd_config_load_s);
    websocket_register_command(&cmd_config_save_s);
    websocket_register_command(&cmd_config_set_s);
}