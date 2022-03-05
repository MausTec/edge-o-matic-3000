#include "VERSION.h"
#include "api/index.h"
#include "eom-hal.h"
#include "system/websocket_handler.h"

static command_err_t cmd_system_restart(cJSON* command, cJSON* response,
                                        websocket_client_t* client) {
    fflush(stdout);
    esp_restart();
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

static command_err_t cmd_system_info(cJSON* command, cJSON* response, websocket_client_t* client) {
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

static command_err_t cmd_system_stream_readings(cJSON* command, cJSON* response,
                                                websocket_client_t* client) {
    if (client != NULL) {
        client->broadcast_flags |= WS_BROADCAST_READINGS;
        client->broadcast_flags |= WS_BROADCAST_SYSTEM;
        return CMD_OK;
    } else {
        return CMD_NOT_FOUND;
    }
}

static const websocket_command_t cmd_system_stream_readings_s = {
    .command = "streamReadings",
    .func = &cmd_system_stream_readings,
};

void api_register_system(void) {
    websocket_register_command(&cmd_system_restart_s);
    websocket_register_command(&cmd_system_time_s);
    websocket_register_command(&cmd_system_info_s);
    websocket_register_command(&cmd_system_stream_readings_s);
}