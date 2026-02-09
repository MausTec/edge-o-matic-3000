#ifndef __drivers__plugin_driver_h
#define __drivers__plugin_driver_h

#ifdef __cplusplus
extern "C" {
#endif

#include "bluetooth_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "host/ble_hs.h"
#include "mt_actions.h"
#include <stdbool.h>

/**
 * @brief Maximum TX buffer size for plugin-driven BLE writes
 */
#define PLUGIN_DRIVER_TX_MAX 64

/**
 * @brief State struct attached to both peer->driver_state and plugin->device. ONLY SUPPORTS TX/RX
 * type devices.
 *
 * This is the bridge between an mt-actions plugin and the BLE peer it drives.
 * Created during discover(), and hopefully freed during disconnect().
 */
typedef struct plugin_driver_state {
    peer_t* peer;         // Back-pointer to the BLE peer
    mta_plugin_t* plugin; // Back-pointer to the mt-actions plugin

    // TX characteristics discovered during service discovery
    struct ble_gatt_chr tx_chr; // Characteristic with WRITE property
    bool has_tx_chr;
    struct ble_gatt_chr tx_no_rsp_chr; // Characteristic with WRITE_NO_RSP property
    bool has_tx_no_rsp_chr;

    // Pending TX buffer (queued writes from plugin, flushed in tick)
    char pending_tx[PLUGIN_DRIVER_TX_MAX];
    int pending_len;
    bool pending_tx_flag;  // true = write in flight
    bool use_write_no_rsp; // whether pending write uses no-response
    xSemaphoreHandle tx_mutex;
} plugin_driver_state_t;

/**
 * @brief The plugin driver bridge vtable
 *
 */
extern const bluetooth_driver_t PLUGIN_DRIVER;

/**
 * @brief Initialize the plugin driver bridge
 *
 * Scans loaded mt-actions plugins for type="ble_driver" and registers
 * them for BLE peer matching as plugin instances (this replaces the old native plugin compilation
 * route.)
 */
void plugin_driver_init(void);

#ifdef __cplusplus
}
#endif

#endif
