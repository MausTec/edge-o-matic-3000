#include "actions/ble.h"
#include "drivers/plugin_driver.h"
#include <esp_log.h>
#include <string.h>

static const char* TAG = "actions:ble";

/**
 * @brief Retrieve the BLE connection context from the current scope.
 *
 * The scope's user_data is set to the ble_conn_ctx_t by plugin_driver_try_claim.
 * This function validates that the context is a BLE connection
 *
 * @return ble_conn_ctx_t pointer, or NULL if the scope has no BLE context
 */
static ble_conn_ctx_t* get_ble_ctx(mta_scope_t* scope) {
    ble_conn_ctx_t* ctx = (ble_conn_ctx_t*)mta_scope_get_user_data(scope);
    if (!ctx || !ctx->peer) return NULL;
    return ctx;
}

/**
 * @brief bleWrite(data) - Queue a BLE GATT write (with response) to the plugin's device
 *
 * The data argument is a string. The write is queued into the BLE connection's
 * pending TX buffer and flushed on the next driver tick.
 */
static int
host_ble_write(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count) {
    if (arg_count < 1) return -1;

    ble_conn_ctx_t* ctx = get_ble_ctx(scope);
    if (!ctx) {
        ESP_LOGW(TAG, "bleWrite: no BLE device context");
        return -1;
    }

    if (!ctx->has_tx_chr) {
        ESP_LOGW(TAG, "bleWrite: no writable characteristic");
        return -1;
    }

    const char* data = mta_arg_get_string(plugin, scope, args, 0);
    if (!data) {
        ESP_LOGW(TAG, "bleWrite: data argument is not a string");
        return -1;
    }

    int len = strlen(data);
    if (len >= PLUGIN_DRIVER_TX_MAX) {
        ESP_LOGW(TAG, "bleWrite: data too long (%d >= %d)", len, PLUGIN_DRIVER_TX_MAX);
        len = PLUGIN_DRIVER_TX_MAX - 1;
    }

    if (xSemaphoreTake(ctx->tx_mutex, 1000UL / portTICK_RATE_MS)) {
        memcpy(ctx->pending_tx, data, len);
        ctx->pending_tx[len] = '\0';
        ctx->pending_len = len;
        ctx->use_write_no_rsp = false;
        xSemaphoreGive(ctx->tx_mutex);
        ESP_LOGD(TAG, "bleWrite: queued %d bytes", len);
    } else {
        ESP_LOGW(TAG, "bleWrite: timeout waiting for TX mutex");
        return -1;
    }

    return 0;
}

/**
 * @brief bleWriteNoResponse(data) - Queue a BLE GATT write-without-response
 *
 * Same as bleWrite but uses the write-no-response characteristic.
 */
static int host_ble_write_no_response(
    mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count
) {
    if (arg_count < 1) return -1;

    ble_conn_ctx_t* ctx = get_ble_ctx(scope);
    if (!ctx) {
        ESP_LOGW(TAG, "bleWriteNoResponse: no BLE device context");
        return -1;
    }

    // Prefer the no-rsp characteristic, fall back to regular write chr
    if (!ctx->has_tx_no_rsp_chr && !ctx->has_tx_chr) {
        ESP_LOGW(TAG, "bleWriteNoResponse: no writable characteristic");
        return -1;
    }

    const char* data = mta_arg_get_string(plugin, scope, args, 0);
    if (!data) {
        ESP_LOGW(TAG, "bleWriteNoResponse: data argument is not a string");
        return -1;
    }

    int len = strlen(data);
    if (len >= PLUGIN_DRIVER_TX_MAX) {
        ESP_LOGW(TAG, "bleWriteNoResponse: data too long (%d >= %d)", len, PLUGIN_DRIVER_TX_MAX);
        len = PLUGIN_DRIVER_TX_MAX - 1;
    }

    if (xSemaphoreTake(ctx->tx_mutex, 1000UL / portTICK_RATE_MS)) {
        memcpy(ctx->pending_tx, data, len);
        ctx->pending_tx[len] = '\0';
        ctx->pending_len = len;
        ctx->use_write_no_rsp = true;
        xSemaphoreGive(ctx->tx_mutex);
        ESP_LOGD(TAG, "bleWriteNoResponse: queued %d bytes", len);
    } else {
        ESP_LOGW(TAG, "bleWriteNoResponse: timeout waiting for TX mutex");
        return -1;
    }

    return 0;
}

void action_ble_init(void) {
    mta_register_system_function("bleWrite", host_ble_write, "ble:write");
    mta_register_system_function("bleWriteNoResponse", host_ble_write_no_response, "ble:write");
}
