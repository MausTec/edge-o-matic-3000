#include "api/index.h"
#include "eom-hal.h"
#include "esp_log.h"
#include "orgasm_control.h"
#include "system/websocket_handler.h"

static const char* TAG = "api/edging";

static command_err_t
cmd_edging_set_mode(cJSON* command, cJSON* response, websocket_client_t* client) {
    const char* mode_str = command->valuestring;
    orgasm_output_mode_t mode = orgasm_control_str_to_output_mode(mode_str);

    if (mode >= 0) {
        orgasm_control_set_output_mode(mode);
        return CMD_OK;
    } else {
        return CMD_ARG_ERR;
    }
}

static const websocket_command_t cmd_edging_set_mode_s = {
    .command = "setMode",
    .func = &cmd_edging_set_mode,
};

static command_err_t
cmd_edging_set_motor(cJSON* command, cJSON* response, websocket_client_t* client) {
    int speed = command->valueint;
    if (orgasm_control_get_output_mode() != OC_MANUAL_CONTROL) {
        return CMD_FAIL;
    }
    eom_hal_set_motor_speed(speed);
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