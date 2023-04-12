#ifndef __edge_o_matic__bluetooth_driver_h
#define __edge_o_matic__bluetooth_driver_h

#ifdef __cplusplus
extern "C" {
#endif

#include "bluetooth_manager.h"
#include <stdint.h>

typedef enum bluetooth_driver_status {
    BT_DEVICE_DISCONNECTED = 0,
    BT_DEVICE_CONNECTED,
    BT_DEVICE_ETIMEOUT,
    BT_DEVICE_ENOMEM,
    BT_DEVICE_ECONNECT,
} bluetooth_driver_status_t;

typedef enum bluetooth_driver_compatibility {
    BT_DRIVER_NOT_COMPATIBLE,
    BT_DRIVER_COMPATIBLE,
    BT_DRIVER_DISCOVERY_ERROR,
} bluetooth_driver_compatibility_t;

typedef struct bluetooth_driver {
    bluetooth_driver_status_t (*connect)(peer_t* peer);
    void (*disconnect)(peer_t* peer);
    void (*tick)(peer_t* peer);
    size_t (*get_name)(peer_t* peer, char* buffer, size_t len);
    bluetooth_driver_status_t (*get_status)(peer_t* peer);
    void (*set_speed)(peer_t* peer, uint8_t speed);
    bluetooth_driver_compatibility_t (*discover)(peer_t* peer);
} bluetooth_driver_t;

typedef void (*bt_device_enumeration_cb_t)(peer_t* device, void* arg);

void bluetooth_driver_init(void);
void bluetooth_driver_tick(void);
void bluetooth_driver_broadcast_speed(uint8_t speed);
void bluetooth_driver_broadcast_arousal(uint16_t arousal);
void bluetooth_driver_enumerate_devices(bt_device_enumeration_cb_t cb, void* arg);

void bluetooth_driver_register_peer(peer_t* peer);
void bluetooth_driver_unregister_peer(peer_t* peer);

int bluetooth_driver_sendf(peer_t* peer, struct ble_gatt_chr* chr, const char* fmt, ...);

/**
 * @brief Iterates over known drivers, calling their discover function to determine if they work for
 * the peer.
 *
 * Do know that the Discover function also initializes the driver state, so it's best to ensure that
 * you're cleaning up after this somehow. If it returns a driver, assume that driver has been
 * initialized and go stright to registereing the peer. A future version of me might make that
 * happen automatically.
 *
 * @param peer
 * @return const bluetooth_driver_t*
 */
const bluetooth_driver_t* bluetooth_driver_find_for_peer(peer_t* peer);

#ifdef __cplusplus
}
#endif

#endif
