#include "actions/ble.h"
#include "drivers/plugin_driver.h"
#include <esp_log.h>
#include <string.h>

static const char* TAG = "actions:ble";

/**
 * Retrieve the BLE connection context from the current scope.
 *
 * The scope's user_data is set to the ble_conn_ctx_t by plugin_driver_try_claim.
 * Returns NULL if the scope has no BLE context or the peer is not set.
 */
static ble_conn_ctx_t* get_ble_ctx(mta_scope_t* scope) {
    ble_conn_ctx_t* ctx = (ble_conn_ctx_t*)mta_scope_get_user_data(scope);
    if (!ctx || !ctx->peer) return NULL;
    return ctx;
}

/**
 * Copy the validated data argument into the BLE TX buffer.
 *
 * Caller is responsible for arg validation via mta_read_args before calling.
 * Accepts either a string or a byte-array argument at args[0].
 *
 * @param plugin      Plugin context
 * @param scope       Execution scope (carries BLE connection via user_data)
 * @param args        Validated argument array; args[0] = data
 * @param no_response true = write-without-response characteristic
 * @param fn_name     Function name used in error messages
 * @return 0 on success, -1 on mta_raise error
 */
static int queue_ble_write(
    mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, bool no_response, const char* fn_name
) {
    ble_conn_ctx_t* ctx = get_ble_ctx(scope);
    if (!ctx) {
        return mta_raise(
            plugin, MTA_RT_ERR_HOST_DISPATCH_FAILED, "%s: no BLE device context", fn_name
        );
    }

    if (no_response ? (!ctx->has_tx_no_rsp_chr && !ctx->has_tx_chr) : !ctx->has_tx_chr) {
        return mta_raise(
            plugin, MTA_RT_ERR_HOST_DISPATCH_FAILED, "%s: no writable characteristic", fn_name
        );
    }

    uint8_t buf[PLUGIN_DRIVER_TX_MAX];
    int len = mta_arg_copy_bytes(plugin, scope, args, 0, buf, PLUGIN_DRIVER_TX_MAX - 1);

    if (len < 0) {
        return mta_raise(
            plugin, MTA_RT_ERR_TYPE_MISMATCH, "%s: data must be a string or byte array", fn_name
        );
    }

    if (xSemaphoreTake(ctx->tx_mutex, 1000UL / portTICK_RATE_MS)) {
        memcpy(ctx->pending_tx, buf, len);
        ctx->pending_tx[len] = '\0';
        ctx->pending_len = len;
        ctx->use_write_no_rsp = no_response;

        xSemaphoreGive(ctx->tx_mutex);
        ESP_LOGD(TAG, "%s: queued %d bytes", fn_name, len);
    } else {
        return mta_raise(
            plugin, MTA_RT_ERR_HOST_DISPATCH_FAILED, "%s: timeout waiting for TX mutex", fn_name
        );
    }

    return 0;
}

/**
 * Write data to a BLE GATT characteristic (with response).
 *
 * @plugin ble_write
 * @module ble
 * @arg data:any Data to send (string or byte array)
 * @returns void
 */
static int
host_ble_write(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count) {
    static const mta_arg_spec_t specs[] = {
        { "data", MTA_KIND_ANY },
    };

    mta_argv_t v[1];
    int ret = mta_read_args(plugin, scope, args, arg_count, specs, 1, v, "ble_write");
    if (ret != 0) return ret;

    int rc = queue_ble_write(plugin, scope, args, false, "ble_write");
    if (rc != 0) return rc;

    return mta_return_void(plugin, scope);
}

/**
 * Write data to a BLE GATT characteristic (without response).
 *
 * @plugin ble_write_no_response
 * @module ble
 * @arg data:any Data to send (string or byte array)
 * @returns void
 */
static int host_ble_write_no_response(
    mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count
) {
    static const mta_arg_spec_t specs[] = {
        { "data", MTA_KIND_ANY },
    };

    mta_argv_t v[1];
    int ret = mta_read_args(plugin, scope, args, arg_count, specs, 1, v, "ble_write_no_response");
    if (ret != 0) return ret;

    int rc = queue_ble_write(plugin, scope, args, true, "ble_write_no_response");
    if (rc != 0) return rc;

    return mta_return_void(plugin, scope);
}

void action_ble_init(void) {
    mta_register_system_function("ble_write", host_ble_write, "ble:write");
    mta_register_system_function("ble_write_no_response", host_ble_write_no_response, "ble:write");
}
