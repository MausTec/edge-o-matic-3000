#include "actions/ui.h"
#include "eom-hal.h"
#include "esp_log.h"
#include "ui/control_lock.h"
#include "ui/toast.h"
#include <stdio.h>

#define PERM_UI_LED "ui:led"
#define PERM_UI_NOTIFY "ui:notify"
#define PERM_UI_LOCK "ui:lock"

static const char* TAG = "actions:ui";

/**
 * Set the rotary encoder's RGB status LED.
 *
 * @plugin set_led_color
 * @module ui
 * @arg r:int Red channel (0-255)
 * @arg g:int Green channel (0-255)
 * @arg b:int Blue channel (0-255)
 * @returns void
 */
static int
host_set_led_color(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count) {
    static const mta_arg_spec_t specs[] = {
        { "r", MTA_KIND_INT },
        { "g", MTA_KIND_INT },
        { "b", MTA_KIND_INT },
    };

    mta_argv_t v[3];
    int ret = mta_read_args(plugin, scope, args, arg_count, specs, 3, v, "set_led_color");
    if (ret != 0) return ret;

    ESP_LOGD(TAG, "Setting LED color to R:%d G:%d B:%d", v[0].val.i, v[1].val.i, v[2].val.i);

    eom_hal_set_encoder_rgb((uint8_t)v[0].val.i, (uint8_t)v[1].val.i, (uint8_t)v[2].val.i);
    return mta_return_void(plugin, scope);
}

/**
 * Show a user-visible toast notification on the device screen.
 *
 * @plugin notify
 * @module ui
 * @arg text:string Message to display
 * @returns void
 */
static int
host_notify(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count) {
    static const mta_arg_spec_t specs[] = {
        { "text", MTA_KIND_STRING },
    };

    mta_argv_t v[1];
    int ret = mta_read_args(plugin, scope, args, arg_count, specs, 1, v, "notify");
    if (ret != 0) return ret;

    ESP_LOGD(TAG, "Showing toast notification: %s", v[0].val.s);

    ui_toast("%s", v[0].val.s);
    return mta_return_void(plugin, scope);
}

/**
 * Lock all button/encoder input at the UI layer. The only way out is
 * holding BACK+OK for 5s, or a power cycle.
 *
 * @plugin lock_controls
 * @module ui
 * @arg reason:string Shown in device logs while locked
 * @returns void
 */
static int
host_lock_controls(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count) {
    static const mta_arg_spec_t specs[] = {
        { "reason", MTA_KIND_STRING },
    };

    mta_argv_t v[1];
    int ret = mta_read_args(plugin, scope, args, arg_count, specs, 1, v, "lock_controls");
    if (ret != 0) return ret;

    char reason[64];
    const char* plugin_name = mta_plugin_get_name(plugin);
    snprintf(reason, sizeof(reason), "%s: %s", plugin_name ? plugin_name : "plugin", v[0].val.s);

    ESP_LOGD(TAG, "Locking controls for reason: %s", reason);

    control_lock_engage(reason);
    return mta_return_void(plugin, scope);
}

/**
 * Release a lock previously engaged via lock_controls().
 *
 * @plugin unlock_controls
 * @module ui
 * @returns void
 */
static int
host_unlock_controls(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count) {
    (void)args;
    if (arg_count != 0) {
        return mta_raise(
            plugin,
            MTA_RT_ERR_ARG_COUNT_MISMATCH,
            "unlock_controls: expected no args, got %d",
            arg_count
        );
    }

    ESP_LOGD(TAG, "Releasing control lock");

    control_lock_release();
    return mta_return_void(plugin, scope);
}

void action_ui_init(void) {
    mta_register_system_function("set_led_color", host_set_led_color, PERM_UI_LED);
    mta_register_system_function("notify", host_notify, PERM_UI_NOTIFY);
    mta_register_system_function("lock_controls", host_lock_controls, PERM_UI_LOCK);
    mta_register_system_function("unlock_controls", host_unlock_controls, PERM_UI_LOCK);
}
