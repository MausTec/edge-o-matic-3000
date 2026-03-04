#include "drivers/plugin_driver.h"
#include "mt_actions_internal.h"
#include "system/action_manager.h"
#include "system/event_manager.h"
#include <esp_log.h>
#include <string.h>

static const char* TAG = "plugin_driver";

static int on_tx_complete(
    uint16_t conn_handle, const struct ble_gatt_error* error, struct ble_gatt_attr* attr, void* arg
) {
    ble_conn_ctx_t* ctx = (ble_conn_ctx_t*)arg;
    if (!ctx) return 0;

    if (error->status != 0) {
        ESP_LOGW(TAG, "TX failed: status=%d", error->status);
    }

    ctx->pending_tx_flag = false;
    ctx->tx_retry_count = 0;
    return 0;
}

static void ctx_flush_tx(ble_conn_ctx_t* ctx) {
    if (!ctx || ctx->pending_len == 0 || ctx->pending_tx_flag) return;

    // EBUSY backoff: don't hammer NimBLE while a prior GATT op is finishing
    if (ctx->retry_after && xTaskGetTickCount() < ctx->retry_after) return;

    if (xSemaphoreTake(ctx->tx_mutex, 1000UL / portTICK_RATE_MS)) {
        char buffer[PLUGIN_DRIVER_TX_MAX];
        int len = ctx->pending_len;
        bool use_no_rsp = ctx->use_write_no_rsp;
        memcpy(buffer, ctx->pending_tx, len);
        buffer[len] = '\0';
        ctx->pending_len = 0; // Consumed — new bleWrites can queue fresh data
        xSemaphoreGive(ctx->tx_mutex);

        int rc;
        if (use_no_rsp && ctx->has_tx_no_rsp_chr) {
            rc = ble_gattc_write_no_rsp_flat(
                ctx->peer->conn_handle, ctx->tx_no_rsp_chr.val_handle, buffer, len
            );
        } else if (ctx->has_tx_chr) {
            ctx->pending_tx_flag = true;
            rc = ble_gattc_write_flat(
                ctx->peer->conn_handle, ctx->tx_chr.val_handle, buffer, len, on_tx_complete, ctx
            );
        } else {
            ESP_LOGW(TAG, "flush: no suitable write characteristic");
            return;
        }

        if (rc == 0) {
            ctx->retry_after = 0;
            ctx->tx_retry_count = 0;
            if (use_no_rsp) ctx->pending_tx_flag = false;
            ESP_LOGI(TAG, "TX: %.*s", len, buffer);
        } else if (rc == 7 /* BLE_HS_EBUSY */) {
            ctx->pending_tx_flag = false;
            ctx->tx_retry_count++;

            if (ctx->tx_retry_count <= PLUGIN_DRIVER_TX_MAX_RETRIES) {
                if (xSemaphoreTake(ctx->tx_mutex, pdMS_TO_TICKS(10))) {
                    if (ctx->pending_len == 0) {
                        memcpy(ctx->pending_tx, buffer, len);
                        ctx->pending_len = len;
                        ctx->use_write_no_rsp = use_no_rsp;
                    }
                    xSemaphoreGive(ctx->tx_mutex);
                }
                ctx->retry_after = xTaskGetTickCount() + pdMS_TO_TICKS(50);
                ESP_LOGD(
                    TAG, "BLE busy, retry %d/%d", ctx->tx_retry_count, PLUGIN_DRIVER_TX_MAX_RETRIES
                );
            } else {
                ESP_LOGW(TAG, "Write dropped after %d retries (BLE busy)", ctx->tx_retry_count);
                ctx->tx_retry_count = 0;
                ctx->retry_after = 0;
            }
        } else {
            ESP_LOGW(TAG, "Write failed: rc=%d", rc);
            ctx->pending_tx_flag = false;
        }
    } else {
        ESP_LOGW(TAG, "flush: timeout waiting for TX mutex");
    }
}

/**
 * @brief I/O backend write — queue data into the BLE TX buffer
 *
 * Routes through the async TX pipe. The wait_response parameter selects
 * between write-with-response and write-no-response GATT semantics.
 */
static int ble_io_write(void* device, const void* data, size_t len, bool wait_response) {
    ble_conn_ctx_t* ctx = (ble_conn_ctx_t*)device;
    if (!ctx || !ctx->peer) return -1;

    if (wait_response && !ctx->has_tx_chr) return -1;
    if (!wait_response && !ctx->has_tx_no_rsp_chr && !ctx->has_tx_chr) return -1;

    if ((int)len >= PLUGIN_DRIVER_TX_MAX) {
        ESP_LOGW(TAG, "ble_io_write: data too long (%d >= %d)", (int)len, PLUGIN_DRIVER_TX_MAX);
        len = PLUGIN_DRIVER_TX_MAX - 1;
    }

    if (xSemaphoreTake(ctx->tx_mutex, 1000UL / portTICK_RATE_MS)) {
        memcpy(ctx->pending_tx, data, len);
        ctx->pending_tx[len] = '\0';
        ctx->pending_len = (int)len;
        ctx->use_write_no_rsp = false;
        xSemaphoreGive(ctx->tx_mutex);
        return 0;
    }

    ESP_LOGW(TAG, "ble_io_write: timeout waiting for TX mutex");
    return -1;
}

/**
 * @brief I/O backend close — free the ble_conn_ctx_t
 */
static void ble_io_close(void* device) {
    ble_conn_ctx_t* ctx = (ble_conn_ctx_t*)device;
    if (!ctx) return;

    ESP_LOGI(TAG, "Closing BLE context for '%s'", ctx->peer ? ctx->peer->name : "?");

    if (ctx->peer) {
        ctx->peer->driver_state = NULL;
    }

    if (ctx->tx_mutex) {
        vSemaphoreDelete(ctx->tx_mutex);
    }

    free(ctx);
}

/**
 * @brief Static I/O backend vtable for BLE connections
 */
static const mta_io_backend_t _ble_io_backend = {
    .write = ble_io_write,
    .read = NULL,
    .close = ble_io_close,
};

static void plugin_driver_event_handler(
    const char* event, void* event_arg_ptr, int event_arg_int, void* handler_arg
) {
    (void)event_arg_ptr;
    (void)handler_arg;

    // Map firmware event names to plugin event names
    const char* plugin_event = NULL;

    if (strcmp(event, "EVT_SPEED_CHANGE") == 0) {
        plugin_event = "speedChange";
    } else if (strcmp(event, "EVT_AROUSAL_CHANGE") == 0) {
        plugin_event = "arousalChange";
    }

    if (!plugin_event) return;

    mta_driver_instance_dispatch_all(plugin_event, event_arg_int);
}

void plugin_driver_init(void) {
    ESP_LOGI(TAG, "Plugin driver subsystem initialized");
    event_manager_register_handler(EVT_SPEED_CHANGE, plugin_driver_event_handler, NULL);
    event_manager_register_handler(EVT_AROUSAL_CHANGE, plugin_driver_event_handler, NULL);
}

bool plugin_driver_try_claim(peer_t* peer) {
    size_t plugin_count = action_manager_get_plugin_count();

    for (size_t i = 0; i < plugin_count; i++) {
        mta_plugin_t* plugin = action_manager_get_plugin(i);
        if (!plugin) continue;

        const char* type = mta_plugin_get_type(plugin);
        if (!type || strcmp(type, "ble_driver") != 0) continue;

        // Check if this peer already has an active driver instance
        if (peer->driver_state != NULL) continue;

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

        // GATT service discovery
        int rc = bluetooth_manager_discover_all(peer);
        if (rc != 0) {
            ESP_LOGW(TAG, "Service discovery failed: rc=%d", rc);
            return false;
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
            return false;
        }

        // Allocate BLE connection context
        ble_conn_ctx_t* ctx = (ble_conn_ctx_t*)calloc(1, sizeof(ble_conn_ctx_t));
        if (!ctx) return false;

        ctx->peer = peer;
        ctx->tx_mutex = xSemaphoreCreateMutex();
        ctx->pending_tx_flag = false;
        ctx->pending_len = 0;
        ctx->retry_after = 0;
        ctx->tx_retry_count = 0;

        if (write_chr) {
            ctx->tx_chr = *write_chr;
            ctx->has_tx_chr = true;
        }
        if (write_no_rsp_chr) {
            ctx->tx_no_rsp_chr = *write_no_rsp_chr;
            ctx->has_tx_no_rsp_chr = true;
        }

        // Create driver instance: device = ctx so I/O backend callbacks
        // receive the ble_conn_ctx_t directly. The peer_t* is reachable
        // via ctx->peer, and peer->driver_state points back to the instance.
        mta_driver_instance_t* instance = mta_driver_instance_create(plugin, ctx, &_ble_io_backend);
        if (!instance) {
            vSemaphoreDelete(ctx->tx_mutex);
            free(ctx);
            return false;
        }

        // Set scope user_data to the ble_conn_ctx_t so bleWrite can find the TX pipe
        mta_scope_set_user_data(mta_driver_instance_get_scope(instance), ctx);

        // Cross-link: peer -> driver instance
        peer->driver_state = (void*)instance;

        // Register in global instance list
        mta_driver_instance_add(instance);

        // Fire connect event
        mta_driver_instance_dispatch(instance, "connect", 0, NULL);

        ESP_LOGI(
            TAG,
            "Connection bound: write=%s, write_no_rsp=%s",
            write_chr ? "yes" : "no",
            write_no_rsp_chr ? "yes" : "no"
        );

        return true;
    }

    return false;
}

void plugin_driver_release_peer(peer_t* peer) {
    if (!peer) return;

    mta_driver_instance_t* instance = (mta_driver_instance_t*)peer->driver_state;
    if (!instance) {
        ESP_LOGW(TAG, "release_peer: peer '%s' has no driver instance", peer->name);
        return;
    }

    ESP_LOGI(TAG, "Releasing connection for '%s'", peer->name);

    // destroy fires disconnect event, calls io_backend->close (frees ctx),
    // and removes from the global instance list.
    mta_driver_instance_destroy(instance);
}

static void tick_flush_cb(mta_driver_instance_t* instance, void* arg) {
    (void)arg;
    ble_conn_ctx_t* ctx = (ble_conn_ctx_t*)mta_driver_instance_get_device(instance);
    if (!ctx) return;
    ctx_flush_tx(ctx);
}

void plugin_driver_tick(void) {
    mta_driver_instance_foreach(tick_flush_cb, NULL);
}

typedef struct {
    plugin_driver_enum_cb_t cb;
    void* arg;
} enum_ctx_t;

static void enum_cb(mta_driver_instance_t* instance, void* arg) {
    enum_ctx_t* ectx = (enum_ctx_t*)arg;
    ble_conn_ctx_t* ctx = (ble_conn_ctx_t*)mta_driver_instance_get_device(instance);
    if (ctx && ctx->peer) ectx->cb(ctx->peer, ectx->arg);
}

void plugin_driver_enumerate(plugin_driver_enum_cb_t cb, void* arg) {
    if (!cb) return;
    enum_ctx_t ectx = { .cb = cb, .arg = arg };
    mta_driver_instance_foreach(enum_cb, &ectx);
}

bool plugin_driver_can_handle_name(const char* name) {
    if (!name) return false;

    size_t plugin_count = action_manager_get_plugin_count();

    for (size_t i = 0; i < plugin_count; i++) {
        mta_plugin_t* plugin = action_manager_get_plugin(i);
        if (!plugin) continue;

        const char* type = mta_plugin_get_type(plugin);
        if (!type || strcmp(type, "ble_driver") != 0) continue;

        const char* ble_name = mta_plugin_get_match_ble_name(plugin);
        const char* ble_prefix = mta_plugin_get_match_ble_name_prefix(plugin);

        if (ble_name && strcmp(name, ble_name) == 0) return true;
        if (ble_prefix && strncmp(name, ble_prefix, strlen(ble_prefix)) == 0) return true;
    }

    return false;
}
