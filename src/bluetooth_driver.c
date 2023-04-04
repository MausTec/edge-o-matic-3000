#include "bluetooth_driver.h"
#include "drivers/index.h"
#include "esp_log.h"

static const char* TAG = "bluetooth_driver";

static struct bt_driver_peer_node {
    peer_t peer;
    struct bt_driver_peer_node* next;
}* _peers = NULL;

void bluetooth_driver_init(void);

void bluetooth_driver_broadcast_speed(uint8_t speed) {
    struct bt_driver_peer_node* node = _peers;

    while (node != NULL) {
        const bluetooth_driver_t* driver = node->peer.driver;
        if (driver != NULL) driver->set_speed(&node->peer, speed);
        node = node->next;
    }
}

void bluetooth_driver_broadcast_arousal(uint16_t arousal) {
    struct bt_driver_peer_node* node = _peers;

    while (node != NULL) {
        const bluetooth_driver_t* driver = node->peer.driver;
        // if (driver != NULL) driver->set_arousal(node->next, speed);
        node = node->next;
    }
}

void bluetooth_driver_register_peer(peer_t* peer) {
    const bluetooth_driver_t* driver = peer->driver;

    if (driver == NULL) {
        ESP_LOGE(TAG, "Tried to register a peer without a driver. This is useless to me.");
        return;
    }

    struct bt_driver_peer_node* node =
        (struct bt_driver_peer_node*)malloc(sizeof(struct bt_driver_peer_node));

    if (node == NULL) {
        ESP_LOGE(TAG, "Failed to malloc driver node.");
        return;
    }

    node->peer = *peer;
    node->next = _peers;
    _peers = node;

    driver->connect(peer);
}

void bluetooth_driver_unregister_peer(peer_t* peer) {
    const bluetooth_driver_t* driver = peer->driver;

    if (driver == NULL) {
        ESP_LOGE(TAG, "Tried to unregister a peer without a driver. How?");
        return;
    }

    driver->disconnect(peer);
    // remove from ll
}

int bluetooth_driver_sendf(peer_t* peer, struct ble_gatt_chr* chr, const char* fmt, ...) {
    char buffer[100];

    va_list argptr;
    va_start(argptr, fmt);
    int len = vsniprintf(buffer, 100, fmt, argptr);
    va_end(argptr);

    ESP_LOGI(TAG, "BLE Driver sendf: %s, %d bytes", buffer, len);
    int rc = ble_gattc_write_flat(peer->conn_handle, chr->val_handle, buffer, len, NULL, NULL);
    return rc;
}

const bluetooth_driver_t* bluetooth_driver_find_for_peer(peer_t* peer) {
    for (size_t i = 0; i < sizeof(BT_DRIVERS) / sizeof(BT_DRIVERS[0]); i++) {
        const bluetooth_driver_t* driver = BT_DRIVERS[i];
        if (driver->discover == NULL) continue;

        bluetooth_driver_compatibility_t compat = driver->discover(peer);

        if (compat == BT_DRIVER_COMPATIBLE) {
            return driver;
        }
    }

    return NULL;
}

void bluetooth_driver_init(void) {
}

void bluetooth_driver_tick(void) {
    struct bt_driver_peer_node* node = _peers;

    while (node != NULL) {
        const bluetooth_driver_t* driver = node->peer.driver;
        if (driver != NULL && driver->tick != NULL) driver->tick(&node->peer);
        node = node->next;
    }
}