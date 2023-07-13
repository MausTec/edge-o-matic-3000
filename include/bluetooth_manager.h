#ifndef __edge_o_matic__bluetooth_manager_h
#define __edge_o_matic__bluetooth_manager_h

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "host/ble_hs.h"
#include <stdint.h>

struct peer_chr_node {
    struct ble_gatt_chr chr;
    struct peer_chr_node* next;
};

struct peer_svc_node {
    struct ble_gatt_svc svc;
    struct peer_chr_node* chrs;
    struct peer_svc_node* next;
};

typedef struct peer_discovery {
    struct peer_svc_node* svcs;
    struct peer_svc_node* disc_svc_next;
    int disc_svc_rc;
    SemaphoreHandle_t discover_sem;
} peer_discovery_t;

// TODO: Refactor the discovery into a discovery node, allow it to be freed afterward with
// peer_free_discovery or smth. Work on cleanup.
typedef struct peer {
    ble_addr_t addr;
    char name[BLE_HS_ADV_MAX_SZ];
    uint16_t conn_handle;
    const struct bluetooth_driver* driver;
    void* driver_state;

    struct peer_svc_node* svcs;
    struct peer_svc_node* disc_svc_next;
    int disc_svc_rc;
    SemaphoreHandle_t discover_sem;
} peer_t;

typedef void (*bluetooth_manager_scan_callback_t)(peer_t* peer, void* arg);

void bluetooth_manager_init(void);
void bluetooth_manager_deinit(void);

void bluetooth_manager_start_advertising(void);
void bluetooth_manager_stop_advertising(void);
void bluetooth_manager_start_scan(bluetooth_manager_scan_callback_t on_result, void* arg);
int bluetooth_manager_stop_scan(void);
int bluetooth_manager_connect(peer_t* peer);
int bluetooth_manager_disconnect(peer_t* peer);
int bluetooth_manager_discover_all(peer_t* peer);

#ifdef __cplusplus
}
#endif

#endif
