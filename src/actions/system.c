#include "actions/system.h"
#include <esp_log.h>
#include <esp_random.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "actions:system";

/**
 * Pause plugin execution for a given number of milliseconds.
 *
 * @plugin delay
 * @module system
 * @arg ms:int Delay duration in milliseconds
 */
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

/**
 * Log a message to the device console.
 *
 * Accepts a string or integer argument. The plugin name is prepended
 * for identification.
 *
 * @plugin log
 * @module system
 * @arg msg:any Message or integer value to log
 */
static int action_system_log(
    struct mta_plugin* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count
) {
    if (arg_count < 1) return -1;

    const char* msg = mta_arg_get_string(plugin, scope, args, 0);
    const char* plugin_name = mta_plugin_get_name(plugin);

    if (msg) {
        ESP_LOGI(TAG, "[%s] %s", plugin_name ? plugin_name : "plugin", msg);
    } else {
        // If not a string, log the integer value
        int val = mta_arg_get_int(plugin, scope, args, 0);
        ESP_LOGI(TAG, "[%s] %d", plugin_name ? plugin_name : "plugin", val);
    }

    return 0;
}

/**
 * Generate a random integer using the hardware RNG.
 *
 * With 0 args returns a raw uint32. With 1 arg returns [0, arg).
 * With 2 args returns [arg0, arg1].
 *
 * @plugin random
 * @module system
 * @arg min:int? Lower bound (or upper bound if only one arg) (optional)
 * @arg max:int? Upper bound (optional)
 * @returns int Random integer in the specified range
 */
static int action_system_random(
    struct mta_plugin* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count
) {
    int min_val = 0;
    int max_val = 0;

    if (arg_count == 1) {
        max_val = mta_arg_get_int(plugin, scope, args, 0);
        if (max_val <= 0) {
            mta_return_int(plugin, scope, 0);
            return 0;
        }
        mta_return_int(plugin, scope, (int)(esp_random() % (uint32_t)max_val));
    } else if (arg_count >= 2) {
        min_val = mta_arg_get_int(plugin, scope, args, 0);
        max_val = mta_arg_get_int(plugin, scope, args, 1);

        if (max_val <= min_val) {
            mta_return_int(plugin, scope, min_val);
            return 0;
        }

        uint32_t range = (uint32_t)(max_val - min_val + 1);
        mta_return_int(plugin, scope, min_val + (int)(esp_random() % range));
    } else {
        mta_return_int(plugin, scope, (int)esp_random());
    }

    return 0;
}

/**
 * Get milliseconds elapsed since device boot.
 *
 * Uses esp_timer_get_time() (microseconds) divided by 1000.
 *
 * @plugin millis
 * @module system
 * @returns int Milliseconds since boot
 */
static int action_system_millis(
    struct mta_plugin* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count
) {
    int64_t us = esp_timer_get_time();
    mta_return_int(plugin, scope, (int32_t)(us / 1000));
    return 0;
}

void actions_register_system(void) {
    mta_register_system_function("delay", action_system_delay, NULL);
    mta_register_system_function("log", action_system_log, NULL);
    mta_register_system_function("random", action_system_random, NULL);
    mta_register_system_function("millis", action_system_millis, NULL);
}