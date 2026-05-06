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
    mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count
) {
    static const mta_arg_spec_t specs[] = {
        { "ms", MTA_KIND_INT },
    };

    mta_argv_t v[1];
    int ret = mta_read_args(plugin, scope, args, arg_count, specs, 1, v, "delay");
    if (ret != 0) return ret;

    ESP_LOGI(TAG, "delay(%d) start", v[0].val.i);
    vTaskDelay(v[0].val.i / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "delay done");

    return mta_return_void(plugin, scope);
}

/**
 * Log a message to the device console.
 *
 * The plugin name is prepended for identification.
 *
 * @plugin log
 * @module system
 * @arg msg:any Message to log (string, int, or float)
 */
static int
action_system_log(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count) {
    static const mta_arg_spec_t specs[] = {
        { "msg", MTA_KIND_ANY },
    };

    mta_argv_t v[1];
    int ret = mta_read_args(plugin, scope, args, arg_count, specs, 1, v, "log");
    if (ret != 0) return ret;

    const char* plugin_name = mta_plugin_get_name(plugin);
    mta_arg_type_t type = mta_arg_get_type(plugin, scope, args, 0);

    if (type == MTA_ARG_STRING_REF) {
        ESP_LOGI(
            TAG,
            "[%s] %s",
            plugin_name ? plugin_name : "plugin",
            mta_arg_get_string(plugin, scope, args, 0)
        );
    } else if (type == MTA_ARG_FLOAT) {
        ESP_LOGI(
            TAG,
            "[%s] %g",
            plugin_name ? plugin_name : "plugin",
            mta_arg_get_float(plugin, scope, args, 0)
        );
    } else {
        ESP_LOGI(
            TAG,
            "[%s] %d",
            plugin_name ? plugin_name : "plugin",
            mta_arg_get_int(plugin, scope, args, 0)
        );
    }

    return mta_return_void(plugin, scope);
}

/**
 * Generate a random integer using the hardware RNG.
 *
 * 0 args: raw uint32. 1 arg: [0, hi). 2 args: [lo, hi].
 *
 * @plugin random
 * @module system
 * @arg lo:int? Lower bound (or upper bound if only one arg)
 * @arg hi:int? Upper bound
 * @returns int Random integer in the specified range
 */
static int
action_system_random(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count) {
    // Optional args are handled by arg length checking. This would have to get ported to mt-sdk so
    // it is aware.
    if (arg_count > 2) {
        return mta_raise(
            plugin, MTA_RT_ERR_ARG_COUNT_MISMATCH, "random: expected 0-2 args, got %d", arg_count
        );
    }

    static const mta_arg_spec_t specs[] = {
        { "lo", MTA_KIND_INT },
        { "hi", MTA_KIND_INT },
    };

    mta_argv_t v[2];
    int ret = mta_read_args(plugin, scope, args, arg_count, specs, arg_count, v, "random");
    if (ret != 0) return ret;

    if (arg_count == 0) {
        return mta_return_int(plugin, scope, (int)esp_random());
    } else if (arg_count == 1) {
        int hi = v[0].val.i;
        if (hi <= 0) return mta_return_int(plugin, scope, 0);

        return mta_return_int(plugin, scope, (int)(esp_random() % (uint32_t)hi));
    } else {
        int lo = v[0].val.i, hi = v[1].val.i;
        if (hi <= lo) return mta_return_int(plugin, scope, lo);

        return mta_return_int(plugin, scope, lo + (int)(esp_random() % (uint32_t)(hi - lo + 1)));
    }
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
static int
action_system_millis(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count) {
    (void)args;

    // TODO: Update mta validator to accept no args via null arg spec.
    if (arg_count != 0) {
        return mta_raise(
            plugin, MTA_RT_ERR_ARG_COUNT_MISMATCH, "millis: expected no args, got %d", arg_count
        );
    }

    return mta_return_int(plugin, scope, (int32_t)(esp_timer_get_time() / 1000));
}

void actions_register_system(void) {
    mta_register_system_function("delay", action_system_delay, NULL);
    mta_register_system_function("log", action_system_log, NULL);
    mta_register_system_function("random", action_system_random, NULL);
    mta_register_system_function("millis", action_system_millis, NULL);
}