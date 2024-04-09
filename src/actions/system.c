#include "actions/system.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "actions:system";

int action_system_delay(struct mta_plugin* plugin, cJSON* args) {
    char* arg = cJSON_Print(args);
    ESP_LOGI(TAG, "fn args: %s", arg);
    free(arg);

    cJSON* ms_json = cJSON_GetObjectItem(args, "ms");
    if (ms_json == NULL) return -1;
    int ms = ms_json->valueint;

    ESP_LOGI(TAG, "delay(%d) start", ms);
    vTaskDelay(ms / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "delay done");

    return 0;
}

void actions_register_system(void) {
    mta_register_system_function("delay", action_system_delay);
}