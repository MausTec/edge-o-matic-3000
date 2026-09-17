#include "actions/arousal.h"
#include "esp_log.h"
#include "orgasm_control.h"

#define PERM_AROUSAL_READ "arousal:read"
#define PERM_AROUSAL_WRITE "arousal:write"

static const char* TAG = "actions:arousal";

static int _no_arg_int(
    mta_plugin_t* plugin, mta_scope_t* scope, uint8_t arg_count, const char* fn_name, int32_t value
) {
    if (arg_count != 0) {
        return mta_raise(
            plugin,
            MTA_RT_ERR_ARG_COUNT_MISMATCH,
            "%s: expected no args, got %d",
            fn_name,
            arg_count
        );
    }

    return mta_return_int(plugin, scope, value);
}

/**
 * Read the current (pull-based) arousal value. Complements the reactive
 * arousal_change event.
 *
 * @plugin get_arousal
 * @module arousal
 * @returns int Current arousal value
 */
static int
host_get_arousal(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count) {
    (void)args;
    ESP_LOGD(TAG, "Getting current arousal value");
    return _no_arg_int(
        plugin, scope, arg_count, "get_arousal", (int32_t)orgasm_control_get_arousal()
    );
}

/**
 * Read the most recent raw pressure sensor reading.
 *
 * @plugin get_pressure
 * @module arousal
 * @returns int Most recent raw pressure reading
 */
static int
host_get_pressure(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count) {
    (void)args;
    ESP_LOGD(TAG, "Getting most recent raw pressure sensor reading");
    return _no_arg_int(
        plugin, scope, arg_count, "get_pressure", (int32_t)orgasm_control_get_last_pressure()
    );
}

/**
 * Read the smoothed/averaged pressure reading.
 *
 * @plugin get_pressure_average
 * @module arousal
 * @returns int Averaged pressure reading
 */
static int host_get_pressure_average(
    mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count
) {
    (void)args;
    ESP_LOGD(TAG, "Getting smoothed/averaged pressure reading");
    return _no_arg_int(
        plugin,
        scope,
        arg_count,
        "get_pressure_average",
        (int32_t)orgasm_control_get_average_pressure()
    );
}

/**
 * Read the configured arousal (sensitivity) threshold.
 *
 * @plugin get_arousal_threshold
 * @module arousal
 * @returns int Current sensitivity threshold
 */
static int host_get_arousal_threshold(
    mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count
) {
    (void)args;
    ESP_LOGD(TAG, "Getting configured arousal (sensitivity) threshold");
    return _no_arg_int(
        plugin,
        scope,
        arg_count,
        "get_arousal_threshold",
        (int32_t)orgasm_control_get_arousal_threshold()
    );
}

/**
 * Set the arousal (sensitivity) threshold.
 *
 * @plugin set_arousal_threshold
 * @module arousal
 * @arg threshold:int New threshold value (floored to 10 by firmware)
 * @returns void
 */
static int host_set_arousal_threshold(
    mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count
) {
    static const mta_arg_spec_t specs[] = {
        { "threshold", MTA_KIND_INT },
    };

    mta_argv_t v[1];
    int ret = mta_read_args(plugin, scope, args, arg_count, specs, 1, v, "set_arousal_threshold");
    if (ret != 0) return ret;

    ESP_LOGD(TAG, "Setting new arousal (sensitivity) threshold: %d", v[0].val.i);

    orgasm_control_set_arousal_threshold(v[0].val.i);
    return mta_return_void(plugin, scope);
}

/**
 * Increment the arousal (sensitivity) threshold by a relative amount.
 *
 * @plugin increment_arousal_threshold
 * @module arousal
 * @arg amount:int Amount to add (may be negative)
 * @returns void
 */
static int host_increment_arousal_threshold(
    mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count
) {
    static const mta_arg_spec_t specs[] = {
        { "amount", MTA_KIND_INT },
    };

    mta_argv_t v[1];
    int ret =
        mta_read_args(plugin, scope, args, arg_count, specs, 1, v, "increment_arousal_threshold");
    if (ret != 0) return ret;

    ESP_LOGD(TAG, "Incrementing arousal (sensitivity) threshold by: %d", v[0].val.i);

    orgasm_control_increment_arousal_threshold(v[0].val.i);
    return mta_return_void(plugin, scope);
}

void action_arousal_init(void) {
    mta_register_system_function("get_arousal", host_get_arousal, PERM_AROUSAL_READ);
    mta_register_system_function("get_pressure", host_get_pressure, PERM_AROUSAL_READ);
    mta_register_system_function(
        "get_pressure_average", host_get_pressure_average, PERM_AROUSAL_READ
    );
    mta_register_system_function(
        "get_arousal_threshold", host_get_arousal_threshold, PERM_AROUSAL_READ
    );
    mta_register_system_function(
        "set_arousal_threshold", host_set_arousal_threshold, PERM_AROUSAL_WRITE
    );
    mta_register_system_function(
        "increment_arousal_threshold", host_increment_arousal_threshold, PERM_AROUSAL_WRITE
    );
}
