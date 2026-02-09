#include "actions/ble.h"
#include "drivers/plugin_driver.h"
#include <esp_log.h>
#include <string.h>

static const char* TAG = "actions:ble";

/**
 * @brief bleWrite(data) - Queue a BLE GATT write (with response) to the plugin's device
 *
 * The data argument is a string. The write is queued into the driver bridge's
 * pending TX buffer and flushed on the next driver tick.
 */
static int
host_ble_write(mta_plugin_t* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count) {
    if (arg_count < 1) return -1;

    plugin_driver_state_t* state = (plugin_driver_state_t*)mta_plugin_get_device(plugin);
    if (!state || !state->peer) {
        ESP_LOGW(TAG, "bleWrite: no device context");
        return -1;
    }

    if (!state->has_tx_chr) {
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

    if (xSemaphoreTake(state->tx_mutex, 1000UL / portTICK_RATE_MS)) {
        memcpy(state->pending_tx, data, len);
        state->pending_tx[len] = '\0';
        state->pending_len = len;
        state->use_write_no_rsp = false;
        xSemaphoreGive(state->tx_mutex);
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

    plugin_driver_state_t* state = (plugin_driver_state_t*)mta_plugin_get_device(plugin);
    if (!state || !state->peer) {
        ESP_LOGW(TAG, "bleWriteNoResponse: no device context");
        return -1;
    }

    // Prefer the no-rsp characteristic, fall back to regular write chr
    if (!state->has_tx_no_rsp_chr && !state->has_tx_chr) {
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

    if (xSemaphoreTake(state->tx_mutex, 1000UL / portTICK_RATE_MS)) {
        memcpy(state->pending_tx, data, len);
        state->pending_tx[len] = '\0';
        state->pending_len = len;
        state->use_write_no_rsp = true;
        xSemaphoreGive(state->tx_mutex);
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
