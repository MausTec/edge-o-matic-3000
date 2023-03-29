#include "bluetooth_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char* TAG = "drivers:lovense";

static xSemaphoreHandle discover_sem;

struct lovense_driver_state {
    xSemaphoreHandle notify_sem;
    struct ble_gatt_chr tx_chr;
    struct ble_gatt_chr rx_chr;
};

static void set_speed(peer_t* peer, uint8_t speed);

static bluetooth_driver_status_t connect(peer_t* peer) {

    return BT_DEVICE_CONNECTED;
}

static void _wait_for_notify(peer_t* peer) {
    return; // idk where the notifications come from @todo
    struct lovense_driver_state* state = (struct lovense_driver_state*)peer->driver_state;

    if (!xSemaphoreTake(state->notify_sem, 1000 / portTICK_RATE_MS)) {
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

    int rc = 0;
    rc = bluetooth_driver_sendf(peer, &state->tx_chr, "Vibrate:%d", cmd_arg);

    if (rc == 0) {
        _wait_for_notify(peer);
    }
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
        state->tx_chr = *tx_chr;
        state->rx_chr = *rx_chr;
        peer->driver_state = (void*)state;

        return BT_DRIVER_COMPATIBLE;
    }

    return BT_DRIVER_NOT_COMPATIBLE;
}

const bluetooth_driver_t LOVENSE_DRIVER = {
    .connect = connect,
    .disconnect = disconnect,
    .get_name = get_name,
    .get_status = get_status,
    .set_speed = set_speed,
    .discover = discover,
};