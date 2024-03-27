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

    ESP_LOGI(TAG, "orgasm_control_str_to_output_mode(%s) = %d", mode_str, mode);

    // Legacy compatibility fallback:
    if (mode == _OC_MODE_ERROR) {
        if (!strcasecmp("automatic", mode_str)) {
            mode = OC_AUTOMAITC_CONTROL;
        } else if (!strcasecmp("manual", mode_str)) {
            mode = OC_MANUAL_CONTROL;
        }
    }

    if (mode != _OC_MODE_ERROR) {
        orgasm_control_set_output_mode(mode);
        cJSON_AddStringToObject(response, "mode", orgasm_control_get_output_mode_str());
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
        orgasm_control_set_output_mode(OC_MANUAL_CONTROL);
        cJSON_AddStringToObject(response, "mode", orgasm_control_get_output_mode_str());
    }

    eom_hal_set_motor_speed(speed);
    cJSON_AddNumberToObject(response, "speed", speed);
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