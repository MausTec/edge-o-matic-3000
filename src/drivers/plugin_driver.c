#include "drivers/plugin_driver.h"
#include "system/action_manager.h"
#include <esp_log.h>
#include <string.h>

static const char* TAG = "drivers:plugin";

/**
 * @brief Callback for BLE GATT write completion, clears pending flag and logs result.
 *
 * @param conn_handle
 * @param error
 * @param attr
 * @param arg
 * @return int
 */
static int on_tx_complete(
    uint16_t conn_handle, const struct ble_gatt_error* error, struct ble_gatt_attr* attr, void* arg
) {
    plugin_driver_state_t* state = (plugin_driver_state_t*)arg;
    if (!state) return 0;

    if (error->status == 0) {
        ESP_LOGD(TAG, "TX complete");
        state->pending_tx_flag = false;
        state->pending_len = 0;
    } else {
        ESP_LOGW(TAG, "TX failed: status=%d", error->status);
        state->pending_tx_flag = false;
    }

    return 0;
}

static bluetooth_driver_status_t plugin_connect(peer_t* peer) {
    plugin_driver_state_t* state = (plugin_driver_state_t*)peer->driver_state;
    if (!state || !state->plugin) return BT_DEVICE_ECONNECT;

    ESP_LOGI(TAG, "Plugin driver connect: %s", peer->name);
    mta_event_invoke(state->plugin, "connect", 0);
    return BT_DEVICE_CONNECTED;
}

static void plugin_disconnect(peer_t* peer) {
    plugin_driver_state_t* state = (plugin_driver_state_t*)peer->driver_state;
    if (!state) return;

    ESP_LOGI(TAG, "Plugin driver disconnect: %s", peer->name);

    if (state->plugin) {
        mta_event_invoke(state->plugin, "disconnect", 0);
        mta_plugin_set_device(state->plugin, NULL);
    }

    if (state->tx_mutex) {
        vSemaphoreDelete(state->tx_mutex);
    }

    free(state);
    peer->driver_state = NULL;
}

static void plugin_tick(peer_t* peer) {
    plugin_driver_state_t* state = (plugin_driver_state_t*)peer->driver_state;
    if (!state || state->pending_len == 0 || state->pending_tx_flag) return;

    if (xSemaphoreTake(state->tx_mutex, 1000UL / portTICK_RATE_MS)) {
        char buffer[PLUGIN_DRIVER_TX_MAX];
        int len = state->pending_len;
        memcpy(buffer, state->pending_tx, len);
        buffer[len] = '\0';
        xSemaphoreGive(state->tx_mutex);

        state->pending_tx_flag = true;

        int rc;
        if (state->use_write_no_rsp && state->has_tx_no_rsp_chr) {
            rc = ble_gattc_write_no_rsp_flat(
                peer->conn_handle, state->tx_no_rsp_chr.val_handle, buffer, len
            );
            // No callback for write-no-response, clear immediately
            if (rc == 0) {
                state->pending_tx_flag = false;
                state->pending_len = 0;
            }
        } else if (state->has_tx_chr) {
            rc = ble_gattc_write_flat(
                peer->conn_handle, state->tx_chr.val_handle, buffer, len, on_tx_complete, state
            );
        } else {
            ESP_LOGW(TAG, "tick: no suitable write characteristic");
            state->pending_tx_flag = false;
            return;
        }

        if (rc == 0) {
            ESP_LOGD(TAG, "Write initiated: %d bytes", len);
        } else {
            ESP_LOGW(TAG, "Write failed: rc=%d", rc);
            state->pending_tx_flag = false;
        }
    } else {
        ESP_LOGW(TAG, "tick: timeout waiting for TX mutex");
    }
}

static size_t plugin_get_name(peer_t* peer, char* buffer, size_t len) {
    plugin_driver_state_t* state = (plugin_driver_state_t*)peer->driver_state;
    if (!state || !state->plugin) return 0;

    const char* name = mta_plugin_get_name(state->plugin);
    if (!name) return 0;

    size_t name_len = strlen(name);
    if (name_len >= len) name_len = len - 1;
    memcpy(buffer, name, name_len);
    buffer[name_len] = '\0';
    return name_len;
}

static bluetooth_driver_status_t plugin_get_status(peer_t* peer) {
    return BT_DEVICE_DISCONNECTED;
}

static void plugin_set_speed(peer_t* peer, uint8_t speed) {
    plugin_driver_state_t* state = (plugin_driver_state_t*)peer->driver_state;
    if (!state || !state->plugin) return;

    mta_event_invoke(state->plugin, "speedChange", (int)speed);
}

/**
 * @brief Try to match a BLE peer against all loaded ble_driver plugins.
 *
 * Iterates action_manager plugins looking for type="ble_driver" with a
 * bleName or bleNamePrefix match. If found, performs GATT service discovery
 * and populates the driver state.
 */
static bluetooth_driver_compatibility_t plugin_discover(peer_t* peer) {
    size_t plugin_count = action_manager_get_plugin_count();

    for (size_t i = 0; i < plugin_count; i++) {
        mta_plugin_t* plugin = action_manager_get_plugin(i);
        if (!plugin) continue;

        // Only match ble_driver type plugins
        const char* type = mta_plugin_get_type(plugin);
        if (!type || strcmp(type, "ble_driver") != 0) continue;

        // Already bound to a device?
        if (mta_plugin_get_device(plugin) != NULL) continue;

        // Check BLE name match
        const char* ble_name = mta_plugin_get_match_ble_name(plugin);
        const char* ble_prefix = mta_plugin_get_match_ble_name_prefix(plugin);
        bool matched = false;

        if (ble_name && strcmp(peer->name, ble_name) == 0) {
            matched = true;
        } else if (ble_prefix && strncmp(peer->name, ble_prefix, strlen(ble_prefix)) == 0) {
            matched = true;
        }

        if (!matched) continue;

        ESP_LOGI(
            TAG,
            "Plugin '%s' matched peer '%s'",
            mta_plugin_get_name(plugin) ? mta_plugin_get_name(plugin) : "?",
            peer->name
        );

        // Discover GATT services
        int rc = bluetooth_manager_discover_all(peer);
        if (rc != 0) {
            ESP_LOGW(TAG, "Service discovery failed: rc=%d", rc);
            return BT_DRIVER_DISCOVERY_ERROR;
        }

        // Scan for write characteristics
        struct ble_gatt_chr* write_chr = NULL;
        struct ble_gatt_chr* write_no_rsp_chr = NULL;

        struct peer_svc_node* svc = peer->svcs;
        while (svc != NULL) {
            struct peer_chr_node* chr = svc->chrs;
            while (chr != NULL) {
                uint8_t prop = chr->chr.properties;

                if ((prop & BLE_GATT_CHR_PROP_WRITE) && !write_chr) {
                    write_chr = &chr->chr;
                }

                if ((prop & BLE_GATT_CHR_PROP_WRITE_NO_RSP) && !write_no_rsp_chr) {
                    write_no_rsp_chr = &chr->chr;
                }

                chr = chr->next;
            }
            svc = svc->next;
        }

        if (!write_chr && !write_no_rsp_chr) {
            ESP_LOGW(TAG, "No writable characteristics found for '%s'", peer->name);
            return BT_DRIVER_NOT_COMPATIBLE;
        }

        // Allocate bridge state
        plugin_driver_state_t* state =
            (plugin_driver_state_t*)calloc(1, sizeof(plugin_driver_state_t));
        if (!state) return BT_DRIVER_DISCOVERY_ERROR;

        state->peer = peer;
        state->plugin = plugin;
        state->tx_mutex = xSemaphoreCreateMutex();
        state->pending_tx_flag = false;
        state->pending_len = 0;

        if (write_chr) {
            state->tx_chr = *write_chr;
            state->has_tx_chr = true;
        }

        if (write_no_rsp_chr) {
            state->tx_no_rsp_chr = *write_no_rsp_chr;
            state->has_tx_no_rsp_chr = true;
        }

        // Cross-link: plugin / peer
        peer->driver_state = (void*)state;
        mta_plugin_set_device(plugin, (void*)state);

        ESP_LOGI(
            TAG,
            "Plugin driver bound: write=%s, write_no_rsp=%s",
            write_chr ? "yes" : "no",
            write_no_rsp_chr ? "yes" : "no"
        );

        return BT_DRIVER_COMPATIBLE;
    }

    return BT_DRIVER_NOT_COMPATIBLE;
}

const bluetooth_driver_t PLUGIN_DRIVER = {
    .connect = plugin_connect,
    .disconnect = plugin_disconnect,
    .tick = plugin_tick,
    .get_name = plugin_get_name,
    .get_status = plugin_get_status,
    .set_speed = plugin_set_speed,
    .discover = plugin_discover,
};

void plugin_driver_init(void) {
    ESP_LOGI(TAG, "Plugin driver bridge initialized");
}
