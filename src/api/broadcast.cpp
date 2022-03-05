#include "api/broadcast.h"
#include "cJSON.h"
#include "config_defs.h"
#include "system/websocket_handler.h"

// Evil CPP deps
#include "page.h"

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

    char* mode = NULL;
    switch (RunGraphPage.getMode()) {
    case RGMode::Automatic:
        mode = "Automatic";
        break;
    case RGMode::Manual:
        mode = "Manual";
        break;
    case RGMode::PostOrgasm:
        mode = "PostOrgasm";
        break;
    default:
        mode = "";
    }

    cJSON_AddNumberToObject(root, "pressure", OrgasmControl::getLastPressure());
    cJSON_AddNumberToObject(root, "pavg", OrgasmControl::getAveragePressure());
    cJSON_AddNumberToObject(root, "motor", Hardware::getMotorSpeed());
    cJSON_AddNumberToObject(root, "arousal", OrgasmControl::getArousal());
    cJSON_AddNumberToObject(root, "millis", esp_timer_get_time() / 1000);

    // Everything around this is deprecated and should be moved into its own broadcast.
    cJSON_AddStringToObject(root, "runMode", mode);
    cJSON_AddBoolToObject(root, "permitOrgasm", OrgasmControl::isPermitOrgasmReached());
    cJSON_AddBoolToObject(root, "postOrgasm", OrgasmControl::isPostOrgasmReached());
    cJSON_AddBoolToObject(root, "lock", OrgasmControl::isMenuLocked());

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
