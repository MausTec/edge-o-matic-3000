#include "config.h"
#include "actions/index.h"
#include "mt_actions.h"
#include <string.h>

#define PERM_SYSCFG_READ "syscfg:read"
#define PERM_SYSCFG_WRITE "syscfg:write"

/**
 * Read a system configuration value by key.
 * @TODO This should return typed values.
 *
 * @plugin get_system_config
 * @module config
 * @arg key:string Configuration key path
 * @returns string Configuration value as string
 */
static int
host_get_system_config(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t count) {
    static const mta_arg_spec_t specs[] = {
        { "key", MTA_KIND_STRING },
    };

    mta_argv_t v[1];
    int ret = mta_read_args(plugin, scope, args, count, specs, 1, v, "get_system_config");
    if (ret != 0) return ret;

    char buffer[256];

    // TODO: Figure out config type and return accordingly.
    if (get_config_value(v[0].val.s, buffer, sizeof(buffer))) {
        return mta_return_string(plugin, scope, buffer);
    }

    return mta_raise(
        plugin, MTA_RT_ERR_HOST_DISPATCH_FAILED, "get_system_config: key not found: %s", v[0].val.s
    );
}

/**
 * Write a system configuration value by key.
 *
 * @plugin set_system_config
 * @module config
 * @arg key:string Configuration key path
 * @arg value:string New value to set
 * @returns int 1 if reboot required, 0 otherwise
 */
static int
host_set_system_config(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t count) {
    static const mta_arg_spec_t specs[] = {
        { "key", MTA_KIND_STRING },
        { "value", MTA_KIND_STRING },
    };

    mta_argv_t v[2];
    int ret = mta_read_args(plugin, scope, args, count, specs, 2, v, "set_system_config");
    if (ret != 0) return ret;

    bool require_reboot = false;
    
    if (set_config_value(v[0].val.s, v[1].val.s, &require_reboot)) {
        config_enqueue_save(0);
        return mta_return_int(plugin, scope, require_reboot ? 1 : 0);
    }

    return mta_raise(
        plugin,
        MTA_RT_ERR_HOST_DISPATCH_FAILED,
        "set_system_config: failed to set key: %s",
        v[0].val.s
    );
}

void action_config_init(void) {
    mta_register_system_function("get_system_config", host_get_system_config, PERM_SYSCFG_READ);
    mta_register_system_function("set_system_config", host_set_system_config, PERM_SYSCFG_WRITE);
}
