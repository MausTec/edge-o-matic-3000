#include "actions/motor.h"
#include "esp_log.h"
#include "orgasm_control.h"

#define PERM_MOTOR_READ "motor:read"
#define PERM_MOTOR_WRITE "motor:write"

static const char* TAG = "actions:motor";

static const char* _mode_str(orgasm_output_mode_t mode) {
    switch (mode) {
    case OC_MANUAL_CONTROL: return "manual";
    case OC_AUTOMAITC_CONTROL: return "automatic";
    case OC_PLUGIN_CONTROL: return "plugin";
    default: return "unknown";
    }
}

/**
 * Read the current commanded motor speed.
 *
 * @plugin get_motor_speed
 * @module motor
 * @returns int Current motor speed (0-255)
 */
static int
host_get_motor_speed(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count) {
    (void)args;
    if (arg_count != 0) {
        return mta_raise(
            plugin,
            MTA_RT_ERR_ARG_COUNT_MISMATCH,
            "get_motor_speed: expected no args, got %d",
            arg_count
        );
    }

    ESP_LOGD(TAG, "Getting current commanded motor speed");

    return mta_return_int(plugin, scope, (int32_t)orgasm_control_get_motor_speed());
}

/**
 * Read the current output mode.
 *
 * @plugin get_output_mode
 * @module motor
 * @returns string "manual", "automatic", or "plugin"
 */
static int
host_get_output_mode(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count) {
    (void)args;
    if (arg_count != 0) {
        return mta_raise(
            plugin,
            MTA_RT_ERR_ARG_COUNT_MISMATCH,
            "get_output_mode: expected no args, got %d",
            arg_count
        );
    }

    ESP_LOGD(TAG, "Getting current output mode");

    return mta_return_string(plugin, scope, _mode_str(orgasm_control_get_output_mode()));
}

/**
 * Claim exclusive plugin control of the motor. While held, the firmware's
 * built-in auto-edging state machine stands down and set_motor_speed()
 * becomes effective.
 *
 * @plugin request_motor_control
 * @module motor
 * @returns void
 */
static int host_request_motor_control(
    mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count
) {
    (void)args;
    if (arg_count != 0) {
        return mta_raise(
            plugin,
            MTA_RT_ERR_ARG_COUNT_MISMATCH,
            "request_motor_control: expected no args, got %d",
            arg_count
        );
    }

    ESP_LOGD(TAG, "Requesting exclusive plugin control of the motor");

    orgasm_control_request_plugin_control();
    return mta_return_void(plugin, scope);
}

/**
 * Release plugin motor control, returning the device to manual mode.
 *
 * @plugin release_motor_control
 * @module motor
 * @returns void
 */
static int host_release_motor_control(
    mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count
) {
    (void)args;
    if (arg_count != 0) {
        return mta_raise(
            plugin,
            MTA_RT_ERR_ARG_COUNT_MISMATCH,
            "release_motor_control: expected no args, got %d",
            arg_count
        );
    }

    ESP_LOGD(TAG, "Releasing plugin control of the motor");

    orgasm_control_release_plugin_control();
    return mta_return_void(plugin, scope);
}

/**
 * Directly set the motor speed. Only takes effect while this plugin holds
 * motor control via request_motor_control().
 *
 * @plugin set_motor_speed
 * @module motor
 * @arg speed:int Motor speed, clamped to 0-255
 * @returns void
 */
static int
host_set_motor_speed(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count) {
    static const mta_arg_spec_t specs[] = {
        { "speed", MTA_KIND_INT },
    };

    mta_argv_t v[1];
    int ret = mta_read_args(plugin, scope, args, arg_count, specs, 1, v, "set_motor_speed");
    if (ret != 0) return ret;

    if (!orgasm_control_is_plugin_control()) {
        return mta_raise(
            plugin,
            MTA_RT_ERR_HOST_DISPATCH_FAILED,
            "set_motor_speed: plugin does not hold motor control (call request_motor_control first)"
        );
    }

    ESP_LOGD(TAG, "Setting motor speed to: %d", v[0].val.i);

    int speed = v[0].val.i;
    if (speed < 0) speed = 0;
    if (speed > 255) speed = 255;

    orgasm_control_set_motor_speed_direct((uint8_t)speed);
    return mta_return_void(plugin, scope);
}

void action_motor_init(void) {
    mta_register_system_function("get_motor_speed", host_get_motor_speed, PERM_MOTOR_READ);
    mta_register_system_function("get_output_mode", host_get_output_mode, PERM_MOTOR_READ);
    mta_register_system_function(
        "request_motor_control", host_request_motor_control, PERM_MOTOR_WRITE
    );
    mta_register_system_function(
        "release_motor_control", host_release_motor_control, PERM_MOTOR_WRITE
    );
    mta_register_system_function("set_motor_speed", host_set_motor_speed, PERM_MOTOR_WRITE);
}
