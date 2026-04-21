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
 * Resolve the data argument and optional length override, then queue
 * raw bytes into the BLE TX buffer.
 *
 * Accepts either a string or an MTA array argument for the first parameter.
 * An optional second integer argument overrides the byte count (clamped to
 * the resolved length).
 *
 * @param plugin        Plugin context
 * @param scope         Execution scope (carries BLE connection via user_data)
 * @param args          Argument array: args[0] = data, args[1] = len (optional)
 * @param arg_count     Number of arguments
 * @param no_response   true = write-without-response characteristic
 * @param fn_name       Function name used in log messages
 * @return 0 on success, -1 on error
 */
static int queue_ble_write(
    mta_plugin_t* plugin,
    mta_scope_t* scope,
    mta_arg_t* args,
    uint8_t arg_count,
    bool no_response,
    const char* fn_name
) {
    ble_conn_ctx_t* ctx = get_ble_ctx(scope);
    if (!ctx) {
        ESP_LOGW(TAG, "%s: no BLE device context", fn_name);
        return -1;
    }

    if (no_response ? (!ctx->has_tx_no_rsp_chr && !ctx->has_tx_chr) : !ctx->has_tx_chr) {
        ESP_LOGW(TAG, "%s: no writable characteristic", fn_name);
        return -1;
    }

    uint8_t buf[PLUGIN_DRIVER_TX_MAX];
    int len = mta_arg_copy_bytes(plugin, scope, args, 0, buf, PLUGIN_DRIVER_TX_MAX - 1);
    if (len < 0) {
        ESP_LOGW(TAG, "%s: data argument must be a string or byte array", fn_name);
        return -1;
    }

    // Optional explicit length override
    if (arg_count >= 2) {
        int override_len = mta_arg_get_int(plugin, scope, args, 1);
        if (override_len >= 0 && override_len < len) len = override_len;
    }

    if (xSemaphoreTake(ctx->tx_mutex, 1000UL / portTICK_RATE_MS)) {
        memcpy(ctx->pending_tx, buf, len);
        ctx->pending_tx[len] = '\0';
        ctx->pending_len = len;
        ctx->use_write_no_rsp = no_response;
        xSemaphoreGive(ctx->tx_mutex);
        ESP_LOGD(TAG, "%s: queued %d bytes", fn_name, len);
    } else {
        ESP_LOGW(TAG, "%s: timeout waiting for TX mutex", fn_name);
        return -1;
    }

    return 0;
}

/**
 * Write data to a BLE GATT characteristic (with response).
 *
 * @plugin ble_write
 * @module ble
 * @arg data:string Data to send to the peripheral (string or byte array)
 * @arg len:int? Maximum number of bytes to write (optional)
 * @returns int 0 on success, -1 on failure
 */
static int
host_ble_write(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count) {
    if (arg_count < 1) return -1;
    return queue_ble_write(plugin, scope, args, arg_count, false, "ble_write");
}

/**
 * Write data to a BLE GATT characteristic (without response).
 *
 * @plugin ble_write_no_response
 * @module ble
 * @arg data:string Data to send to the peripheral (string or byte array)
 * @arg len:int? Maximum number of bytes to write (optional)
 * @returns int 0 on success, -1 on failure
 */
static int host_ble_write_no_response(
    mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count
) {
    if (arg_count < 1) return -1;
    return queue_ble_write(plugin, scope, args, arg_count, true, "ble_write_no_response");
}

void action_ble_init(void) {
    mta_register_system_function("ble_write", host_ble_write, "ble:write");
    mta_register_system_function("ble_write_no_response", host_ble_write_no_response, "ble:write");
}
