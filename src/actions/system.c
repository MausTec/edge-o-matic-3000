#include "actions/system.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "actions:system";

int action_system_delay(
    struct mta_plugin* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count
) {
    if (arg_count != 1) {
        ESP_LOGE(TAG, "delay: expected 1 argument, got %d", arg_count);
        return -1;
    }

    int ms = mta_arg_get_int(plugin, scope, args, 0);

    ESP_LOGI(TAG, "delay(%d) start", ms);
    vTaskDelay(ms / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "delay done");

    return 0;
}

void actions_register_system(void) {
    mta_register_system_function_by_name("delay", action_system_delay);
}