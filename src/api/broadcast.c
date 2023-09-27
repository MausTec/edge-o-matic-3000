#include "api/broadcast.h"
#include "cJSON.h"
#include "config_defs.h"
#include "eom-hal.h"
#include "esp_timer.h"
#include "orgasm_control.h"
#include "system/websocket_handler.h"

void api_broadcast_config(void) {
    cJSON* payload = cJSON_CreateObject();
    cJSON* root = cJSON_AddObjectToObject(payload, "configList");

    config_to_json(root, &Config);

    websocket_broadcast(payload, WS_BROADCAST_SYSTEM);
    cJSON_Delete(payload);
}

void api_broadcast_readings(void) {
    cJSON* payload = cJSON_CreateObject();
    cJSON* root = cJSON_AddObjectToObject(payload, "readings");

    cJSON_AddNumberToObject(root, "pressure", orgasm_control_getLastPressure());
    cJSON_AddNumberToObject(root, "pavg", orgasm_control_getAveragePressure());
    cJSON_AddNumberToObject(root, "motor", eom_hal_get_motor_speed());
    cJSON_AddNumberToObject(root, "arousal", orgasm_control_getArousal());
    cJSON_AddNumberToObject(root, "millis", esp_timer_get_time() / 1000);

    // Everything around this is deprecated and should be moved into its own broadcast.
    cJSON_AddStringToObject(root, "runMode", orgasm_control_get_output_mode_str());
    cJSON_AddBoolToObject(root, "permitOrgasm", orgasm_control_isPermitOrgasmReached());
    cJSON_AddBoolToObject(root, "postOrgasm", orgasm_control_isPostOrgasmReached());
    cJSON_AddBoolToObject(root, "lock", orgasm_control_isMenuLocked());

    websocket_broadcast(payload, WS_BROADCAST_READINGS);
    cJSON_Delete(payload);
}

void api_broadcast_storage_status(void) {
    cJSON* payload = cJSON_CreateObject();
    cJSON* root = cJSON_AddObjectToObject(payload, "sdStatus");

    long long size = eom_hal_get_sd_size_bytes();
    cJSON_AddNumberToObject(root, "size", size);

    // Deprecated for now???
    cJSON_AddStringToObject(root, "type", "");

    websocket_broadcast(payload, WS_BROADCAST_SYSTEM);
    cJSON_Delete(payload);
}

void api_broadcast_network_status(void) {
    cJSON* payload = cJSON_CreateObject();
    cJSON* root = cJSON_AddObjectToObject(payload, "wifiStatus");

    // TODO: Actually get this from active connection
    cJSON_AddStringToObject(root, "ssid", Config.wifi_ssid);
    cJSON_AddStringToObject(root, "ip", "");
    cJSON_AddNumberToObject(root, "rssi", -1);

    websocket_broadcast(payload, WS_BROADCAST_SYSTEM);
    cJSON_Delete(payload);
}
