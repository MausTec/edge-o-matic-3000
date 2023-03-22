#ifndef __edge_o_matic__bluetooth_driver_h
#define __edge_o_matic__bluetooth_driver_h

/** @todo Please rename me to bluetooth_manager instead thank you */

#ifdef __cplusplus
extern "C" {
#endif

#include "host/ble_hs.h"
#include <stdint.h>

typedef struct peer {
    struct ble_hs_adv_fields* fields;
    char name[BLE_HS_ADV_MAX_SZ];
    uint16_t conn_handle;
} peer_t;

typedef void (*bluetooth_driver_scan_callback_t)(peer_t* peer, void* arg);

void bluetooth_driver_init(void);
void bluetooth_driver_deinit(void);

void bluetooth_driver_start_advertising(void);
void bluetooth_driver_start_scan(bluetooth_driver_scan_callback_t on_result, void* arg);

#ifdef __cplusplus
}
#endif

#endif
