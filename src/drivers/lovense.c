#include "bluetooth_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define LOVENSE_CMD_MAX_LEN 32

static const char* TAG = "drivers:lovense";
static SemaphoreHandle_t discover_sem;

struct lovense_driver_state {
    char pending_tx[LOVENSE_CMD_MAX_LEN];
    int pending_len;
    bool pending_tx_flag;
    SemaphoreHandle_t tx_mutex;
    SemaphoreHandle_t notify_sem;
    struct ble_gatt_chr tx_chr;
    struct ble_gatt_chr rx_chr;
};

static void set_speed(peer_t* peer, uint8_t speed);

static void sendf(peer_t* peer, const char* fmt, ...) {
    struct lovense_driver_state* state = (struct lovense_driver_state*)peer->driver_state;

    if (xSemaphoreTake(state->tx_mutex, 1000UL / portTICK_PERIOD_MS)) {
        va_list argptr;
        va_start(argptr, fmt);
        state->pending_len = vsniprintf(state->pending_tx, LOVENSE_CMD_MAX_LEN, fmt, argptr);
        va_end(argptr);
        ESP_LOGD(TAG, "sendf(%s)", state->pending_tx);
        xSemaphoreGive(state->tx_mutex);
    } else {
        ESP_LOGW(TAG, "Timeout waiting for TX mutex.");
    }
}

static int on_tx_status(
    uint16_t conn_handle, const struct ble_gatt_error* error, struct ble_gatt_attr* attr, void* arg
) {
    peer_t* peer = (peer_t*)arg;
    struct lovense_driver_state* state = (struct lovense_driver_state*)peer->driver_state;

    ESP_LOGI(TAG, "ble_gattc_write returned on_tx_status: %d", error->status);

    if (error->status == 0) {
        state->pending_tx_flag = false;
        state->pending_len = 0;
    }

    return 0;
}

static void tick(peer_t* peer) {
    struct lovense_driver_state* state = (struct lovense_driver_state*)peer->driver_state;
    if (state->pending_len == 0 || state->pending_tx_flag) return;

    if (xSemaphoreTake(state->tx_mutex, 1000UL / portTICK_PERIOD_MS)) {
        ESP_LOGI(TAG, "BLE Driver sendf: %s, %d bytes", state->pending_tx, state->pending_len);
        char buffer[state->pending_len + 1];
        int len = state->pending_len;
        strcpy(buffer, state->pending_tx);
        xSemaphoreGive(state->tx_mutex);

        // The pending TX flag will block attempts to retry while an operation is in progress.
        state->pending_tx_flag = true;

        int rc = ble_gattc_write_flat(
            peer->conn_handle, state->tx_chr.val_handle, buffer, len, on_tx_status, peer
        );

        if (rc == 0) {
            ESP_LOGI(TAG, "Initiatated write: %d", rc);
        } else {
            ESP_LOGW(TAG, "Failed to write: %d", rc);
        }
    } else {
        ESP_LOGE(TAG, "Timeout waiting for tx mutex!");
    }
}

static bluetooth_driver_status_t connect(peer_t* peer) {
    return BT_DEVICE_CONNECTED;
}

static void _wait_for_notify(peer_t* peer) {
    return; // idk where the notifications come from @todo
    struct lovense_driver_state* state = (struct lovense_driver_state*)peer->driver_state;

    if (!xSemaphoreTake(state->notify_sem, 1000 / portTICK_PERIOD_MS)) {
        ESP_LOGW(TAG, "Timeout waiting for notify!");
    }
}

static size_t get_name(peer_t* peer, char* buffer, size_t len) {
    return 0;
}

static bluetooth_driver_status_t get_status(peer_t* peer) {
    return BT_DEVICE_DISCONNECTED;
}

static void set_speed(peer_t* peer, uint8_t speed) {
    struct lovense_driver_state* state = (struct lovense_driver_state*)peer->driver_state;

    // Lovense takes speeds 0..20, and dividing by 13 with the 255 check is good enough
    uint8_t cmd_arg = speed == 255 ? 20 : speed / 13;
    sendf(peer, "Vibrate:%d", cmd_arg);
}

static void disconnect(peer_t* peer) {
    set_speed(peer, 0);
}

static bluetooth_driver_compatibility_t discover(peer_t* peer) {
    int rc = bluetooth_manager_discover_all(peer);

    if (rc != 0) {
        return BT_DRIVER_DISCOVERY_ERROR;
    }

    struct ble_gatt_chr* tx_chr = NULL;
    struct ble_gatt_chr* rx_chr = NULL;
    struct peer_svc_node* svc = peer->svcs;

    while (svc != NULL) {
        struct peer_chr_node* chr = svc->chrs;

        while (chr != NULL) {
            uint8_t prop = chr->chr.properties;

            if (prop & BLE_GATT_CHR_PROP_NOTIFY) {
                if (rx_chr == NULL) rx_chr = &chr->chr;
            }

            if (prop & BLE_GATT_CHR_PROP_WRITE) {
                ESP_LOGI(TAG, "Write Char: %d", chr->chr.val_handle);
                if (tx_chr == NULL) tx_chr = &chr->chr;
            }

            chr = chr->next;
        }

        svc = svc->next;
    }

    if (tx_chr != NULL && rx_chr != NULL) {
        struct lovense_driver_state* state =
            (struct lovense_driver_state*)malloc(sizeof(struct lovense_driver_state));

        if (state == NULL) {
            return BT_DRIVER_DISCOVERY_ERROR;
        }

        state->notify_sem = xSemaphoreCreateBinary();
        state->tx_mutex = xSemaphoreCreateMutex();
        state->tx_chr = *tx_chr;
        state->rx_chr = *rx_chr;
        state->pending_tx_flag = false;
        state->pending_tx[0] = '\0';
        state->pending_len = 0;
        peer->driver_state = (void*)state;

        return BT_DRIVER_COMPATIBLE;
    }

    return BT_DRIVER_NOT_COMPATIBLE;
}

const bluetooth_driver_t LOVENSE_DRIVER = {
    .connect = connect,
    .disconnect = disconnect,
    .tick = tick,
    .get_name = get_name,
    .get_status = get_status,
    .set_speed = set_speed,
    .discover = discover,
};