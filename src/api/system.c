#include "VERSION.h"
#include "api/index.h"
#include "eom-hal.h"
#include "system/websocket_handler.h"


static command_err_t cmd_system_restart(cJSON* command, cJSON* response) {
    // if (argc != 0) { return CMD_ARG_ERR; }
    // fprintf(console->out, "Device going down for restart, like, NOW!\n");
    return CMD_NOT_FOUND;
}

static const websocket_command_t cmd_system_restart_s = {
    .command = "restart",
    .func = &cmd_system_restart,
};

static command_err_t cmd_system_time(cJSON* command, cJSON* response) { return CMD_OK; }

static const websocket_command_t cmd_system_time_s = {
    .command = "time",
    .func = &cmd_system_time,
};

static command_err_t cmd_system_info(cJSON* command, cJSON* response) {
    char buf[64] = {0};
    eom_hal_get_device_serial(buf, 64);

    cJSON_AddStringToObject(response, "device", "Edge-o-Matic 3000");
    cJSON_AddStringToObject(response, "serial", buf);
    cJSON_AddStringToObject(response, "hwVersion", eom_hal_get_version());
    cJSON_AddStringToObject(response, "fwVersion", VERSION);
    return CMD_OK;
}

static const websocket_command_t cmd_system_info_s = {
    .command = "info",
    .func = &cmd_system_info,
};

void api_register_system(void) {
    websocket_register_command(&cmd_system_restart_s);
    websocket_register_command(&cmd_system_time_s);
    websocket_register_command(&cmd_system_info_s);
}