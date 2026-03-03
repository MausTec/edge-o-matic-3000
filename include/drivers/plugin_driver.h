#ifndef __drivers__plugin_driver_h
#define __drivers__plugin_driver_h

#ifdef __cplusplus
extern "C" {
#endif

#include "bluetooth_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "host/ble_hs.h"
#include "mt_actions.h"
#include <stdbool.h>

/**
 * @brief Maximum TX buffer size for plugin-driven BLE writes
 */
#define PLUGIN_DRIVER_TX_MAX 64
#define PLUGIN_DRIVER_TX_MAX_RETRIES 5

/**
 * @brief BLE-specific connection context
 *
 * Owns the GATT characteristic handles and the async TX pipe for a single
 * BLE connection. Attached to an mta_driver_instance_t via the I/O backend
 * context pointer. Freed when the I/O backend close callback fires.
 */
typedef struct ble_conn_ctx {
    peer_t* peer; // BLE peer handle (not owned)

    // TX characteristics discovered during GATT service discovery
    struct ble_gatt_chr tx_chr;
    bool has_tx_chr;
    struct ble_gatt_chr tx_no_rsp_chr;
    bool has_tx_no_rsp_chr;

    // Async TX pipe: plugin writes queue here, flushed in tick
    char pending_tx[PLUGIN_DRIVER_TX_MAX];
    int pending_len;
    bool pending_tx_flag;  // true = GATT write in flight
    bool use_write_no_rsp; // use write-no-response characteristic
    SemaphoreHandle_t tx_mutex;
    TickType_t retry_after; // Tick count before which flush retries are suppressed
    uint8_t tx_retry_count; // Current EBUSY retry count (reset on success)
} ble_conn_ctx_t;

/**
 * @brief Initialize the plugin driver subsystem.
 *
 * Registers event handlers for driver-relevant events (EVT_SPEED_CHANGE, etc.)
 * so that connected driver instances receive events through their persistent
 * scope rather than the generic action_manager broadcast.
 */
void plugin_driver_init(void);

/**
 * @brief Attempt to claim a BLE peer for a loaded ble_driver plugin.
 *
 * Iterates loaded plugins looking for type="ble_driver" with a matching
 * bleName or bleNamePrefix. If found, performs GATT service discovery,
 * creates a ble_conn_ctx_t and an mta_driver_instance_t, and fires the
 * plugin's "connect" event.
 *
 * @param peer BLE peer to claim
 * @return true if a plugin claimed the peer, false if no match
 */
bool plugin_driver_try_claim(peer_t* peer);

/**
 * @brief Release a BLE connection managed by a plugin.
 *
 * Finds the driver instance associated with the peer, removes it from the
 * global list, and destroys it (fires "disconnect" event, frees scope and
 * BLE context).
 *
 * @param peer BLE peer to release
 */
void plugin_driver_release_peer(peer_t* peer);

/**
 * @brief Tick all active plugin connections.
 *
 * Flushes pending TX buffers for each connection. Called from the
 * plugin task loop.
 */
void plugin_driver_tick(void);

/**
 * @brief Enumerate connected plugin-driven BLE peers.
 *
 * Calls cb for each active connection's peer.
 *
 * @param cb Callback receiving each peer
 * @param arg Opaque argument passed through to cb
 */
typedef void (*plugin_driver_enum_cb_t)(peer_t* peer, void* arg);
void plugin_driver_enumerate(plugin_driver_enum_cb_t cb, void* arg);

/**
 * @brief Pre-filter: check if any loaded ble_driver plugin can handle a device name.
 *
 * Used at scan time to avoid showing unpairable devices in the list.
 *
 * @param name Advertised BLE device name
 * @return true if at least one plugin matches
 */
bool plugin_driver_can_handle_name(const char* name);

#ifdef __cplusplus
}
#endif

#endif
