#include "api/index.h"
#include "system/websocket_handler.h"
#include "esp_log.h"
#include "Page.h"

static const char *TAG = "api/edging";

static command_err_t cmd_edging_set_mode(cJSON* command, cJSON* response, websocket_client_t* client) {
    const char* mode = command->valuestring;
    RunGraphPage.setMode(mode);
    return CMD_OK;
}

static const websocket_command_t cmd_edging_set_mode_s = {
    .command = "setMode",
    .func = &cmd_edging_set_mode,
};

static command_err_t cmd_edging_set_motor(cJSON* command, cJSON* response, websocket_client_t* client) {
    int speed = command->valueint;
    Hardware::setMotorSpeed(speed);
    return CMD_OK;
}

static const websocket_command_t cmd_edging_set_motor_s = {
    .command = "setMotor",
    .func = &cmd_edging_set_motor,
};

void api_register_edging(void) {
    websocket_register_command(&cmd_edging_set_mode_s);
    websocket_register_command(&cmd_edging_set_motor_s);
}