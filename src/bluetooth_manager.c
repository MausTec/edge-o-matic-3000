#include "bluetooth_manager.h"
#include "config.h"
#include "console/console.h"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"

static const char* TAG = "bluetooth_manager";
static bluetooth_manager_scan_callback_t _scan_callback;
static void* _scan_arg;
static SemaphoreHandle_t _ble_connect_sem;

static int blecent_gap_event(struct ble_gap_event* event, void* arg);

static void blecent_on_reset(int reason) {
    ESP_LOGE(TAG, "Resetting state; reason=%d", reason);
}

static void blecent_on_sync(void) {
    int rc;

    /* Make sure we have proper identity address set (public preferred) */
    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);
}

static void blecent_host_task(void* param) {
    ESP_LOGI(TAG, "BLE Host Task Started");

    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void bluetooth_manager_init(void) {
    esp_err_t err;

    _ble_connect_sem = xSemaphoreCreateBinary();

    esp_nimble_hci_and_controller_init();
    nimble_port_init();

    ble_hs_cfg.reset_cb = blecent_on_reset;
    ble_hs_cfg.sync_cb = blecent_on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    int rc;

    // rc = peer_init(BLE_MAX_CONNECTRION)

    rc = ble_svc_gap_device_name_set(Config.bt_display_name);
    // ble_store_config_init();

    if (rc != 0) {
        ESP_LOGE(TAG, "Setting BT Name failed: %d", rc);
    }

    nimble_port_freertos_init(blecent_host_task);
}

void bluetooth_manager_deinit(void) {
    vSemaphoreDelete(_ble_connect_sem);

    int rc;

    rc = nimble_port_stop();

    if (rc == 0) {
        nimble_port_deinit();
        esp_nimble_hci_and_controller_deinit();
    } else {
        ESP_LOGE(TAG, "Nimble port stop failed, rc = %d", rc);
        return;
    }
}

/**
 * @brief Converts an address to a string. The returned string is only valid until the next time
 * this function is called.
 *
 * @param address
 * @return const char*
 */
const char* ble_addr_str(uint8_t address[6]) {
    static char addr_str[6 * 2 + 5 + 1];

    sprintf(
        addr_str,
        "%02x:%02x:%02x:%02x:%02x:%02x",
        address[0],
        address[1],
        address[2],
        address[3],
        address[4],
        address[5]
    );

    return addr_str;
}

int bluetooth_manager_connect(peer_t* peer) {
    uint8_t own_addr_type;
    int rc;

    rc = bluetooth_manager_stop_scan();
    if (rc != 0) return rc;

    rc = bluetooth_manager_disconnect(peer);
    if (rc != 0) return rc;

    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error determining address type; rc=%d", rc);
        return rc;
    }

    rc = ble_gap_connect(own_addr_type, &peer->addr, 30000, NULL, blecent_gap_event, (void*)peer);
    if (rc != 0) {
        ESP_LOGE(
            TAG,
            "Failed to connect to device; addr_type=%d, address=%s, rc=%d",
            peer->addr.type,
            ble_addr_str(peer->addr.val),
            rc
        );

        return rc;
    }

    // wait for connect to complete:
    if (!xSemaphoreTake(_ble_connect_sem, portMAX_DELAY)) {
        return BLE_HS_ETIMEOUT;
    }

    return rc;
}

int bluetooth_manager_disconnect(peer_t* peer) {
    int rc;

    ESP_LOGI(TAG, "Disconnecting from Peer: %d", peer->conn_handle);

    // HCI error codes aren't documented, but this is "Connection Terminated by Local Host"
    rc = ble_gap_terminate(peer->conn_handle, 0x16);
    if (rc == BLE_HS_ENOTCONN) rc = 0;

    return rc;
}

void bluetooth_manager_start_advertising(void) {
    int rc = 255;

    // rc = ble_gap_adv_start();

    if (rc != 0) {
        ESP_LOGE(TAG, "ERROR starting advertising; rc=%d", rc);
        return;
    }

    ESP_LOGI(TAG, "Bluetooth now advertising.");
}

void bluetooth_manager_stop_advertising(void) {
    int rc;

    rc = ble_gap_adv_stop();

    if (rc != 0) {
        ESP_LOGE(TAG, "ERROR stopping advertising; rc=%d", rc);
        return;
    }
}

static void blecent_on_disc_complete(const struct peer* peer, int status, void* arg) {

    if (status != 0) {
        /* Service discovery failed.  Terminate the connection. */
        ESP_LOGI(
            TAG,
            "Error: Service discovery failed; status=%d "
            "conn_handle=%d",
            status,
            peer->conn_handle
        );

        ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return;
    }

    /* Service discovery has completed successfully.  Now we have a complete
     * list of services, characteristics, and descriptors that the peer
     * supports.
     */
    ESP_LOGI(
        TAG,
        "Service discovery complete; status=%d "
        "conn_handle=%d",
        status,
        peer->conn_handle
    );

    // Do something with peer?
}

static int blecent_gap_event(struct ble_gap_event* event, void* arg) {
    struct ble_gap_conn_desc desc;
    struct ble_hs_adv_fields fields;
    int rc;

    ESP_LOGI(TAG, "blecent_gap_event(%d)", event->type);

    switch (event->type) {
    case BLE_GAP_EVENT_DISC:
        rc = ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);

        if (rc != 0) {
            return 0;
        }

        if (fields.name_len > 1) {
            ESP_LOGI(TAG, "BLE_GAP_EVENT_DISC: \"%.*s\"", fields.name_len, (char*)fields.name);
            if (_scan_callback != NULL) {

                peer_t peer = { 0 };

                memcpy(&peer.addr, &event->disc.addr, sizeof(ble_addr_t));
                strncpy(peer.name, (char*)fields.name, fields.name_len);

                _scan_callback(&peer, _scan_arg);
            }
        } else {
            ESP_LOGI(
                TAG,
                "BLE_GAP_EVENT_DISC: Incomplete Name! \"%.*s\"",
                fields.name_len,
                (char*)fields.name
            );
        }

        // Do something with this event...
        return 0;

    case BLE_GAP_EVENT_CONNECT:
        /* A new connection was established or a connection attempt failed. */
        if (event->connect.status != 0) {
            /* Connection attempt failed; resume scanning. */
            ESP_LOGI(TAG, "ERROR: Connection failed; status=%d", event->connect.status);
            // blecent_scan();
        } else {
            /* Connection successfully established. */
            ESP_LOGI(TAG, "Connection established ");

            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == 0);

            peer_t* peer = (peer_t*)arg;
            if (peer == NULL) {
                ESP_LOGE(TAG, "NULL Peer in BLE Event.");
                return 0;
            }

            peer->conn_handle = event->connect.conn_handle;

            /* Remember peer. */
            // rc = peer_add(event->connect.conn_handle);
            if (rc != 0) {
                ESP_LOGE(TAG, "Failed to add peer; rc=%d", rc);
                bluetooth_manager_disconnect(peer);
                return 0;
            }

            xSemaphoreGive(_ble_connect_sem);
        }

        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        /* Connection terminated. */
        ESP_LOGI(TAG, "disconnect; reason=%d ", event->disconnect.reason);
        // print_conn_desc(&event->disconnect.conn);
        ESP_LOGI(TAG, "");

        /* Forget about peer. */
        // peer_delete(event->disconnect.conn.conn_handle);

        /* Resume scanning. */
        // blecent_scan();
        return 0;

    case BLE_GAP_EVENT_DISC_COMPLETE:
        ESP_LOGI(TAG, "discovery complete; reason=%d", event->disc_complete.reason);
        return 0;

    case BLE_GAP_EVENT_ENC_CHANGE:
        /* Encryption has been enabled or disabled for this connection. */
        ESP_LOGI(TAG, "encryption change event; status=%d ", event->enc_change.status);
        rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
        assert(rc == 0);
        // print_conn_desc(&desc);
#if CONFIG_EXAMPLE_ENCRYPTION
        /*** Go for service discovery after encryption has been successfully enabled ***/
        rc = peer_disc_all(event->connect.conn_handle, blecent_on_disc_complete, NULL);
        if (rc != 0) {
            ESP_LOGI(TAG, "Failed to discover services; rc=%d", rc);
            return 0;
        }
#endif
        return 0;

    case BLE_GAP_EVENT_NOTIFY_RX:
        /* Peer sent us a notification or indication. */
        ESP_LOGI(
            TAG,
            "received %s; conn_handle=%d attr_handle=%d "
            "attr_len=%d",
            event->notify_rx.indication ? "indication" : "notification",
            event->notify_rx.conn_handle,
            event->notify_rx.attr_handle,
            OS_MBUF_PKTLEN(event->notify_rx.om)
        );

        /* Attribute data is contained in event->notify_rx.om. Use
         * `os_mbuf_copydata` to copy the data received in notification mbuf */
        return 0;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(
            TAG,
            "mtu update event; conn_handle=%d cid=%d mtu=%d",
            event->mtu.conn_handle,
            event->mtu.channel_id,
            event->mtu.value
        );
        return 0;

    case BLE_GAP_EVENT_REPEAT_PAIRING:
        /* We already have a bond with the peer, but it is attempting to
         * establish a new secure link.  This app sacrifices security for
         * convenience: just throw away the old bond and accept the new link.
         */

        /* Delete the old bond. */
        rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        assert(rc == 0);
        ble_store_util_delete_peer(&desc.peer_id_addr);

        /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
         * continue with the pairing operation.
         */
        return BLE_GAP_REPEAT_PAIRING_RETRY;

#if CONFIG_EXAMPLE_EXTENDED_ADV
    case BLE_GAP_EVENT_EXT_DISC:
        /* An advertisment report was received during GAP discovery. */
        ext_print_adv_report(&event->disc);

        blecent_connect_if_interesting(&event->disc);
        return 0;
#endif

    default: return 0;
    }
}

void bluetooth_manager_start_scan(bluetooth_manager_scan_callback_t on_result, void* arg) {
    uint8_t own_addr_type;
    struct ble_gap_disc_params disc_params;
    int rc;

    _scan_callback = on_result;
    _scan_arg = arg;

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);

    if (rc != 0) {
        ESP_LOGI(TAG, "TAG determining address type; rc=%d", rc);
        return;
    }

    /* Tell the controller to filter duplicates; we don't want to process
     * repeated advertisements from the same device.
     */
    disc_params.filter_duplicates = 1;

    /**
     * Perform a passive scan.  I.e., don't send follow-up scan requests to
     * each advertiser.
     */
    disc_params.passive = 1;

    /* Use defaults for the rest of the parameters. */
    disc_params.itvl = 0;
    disc_params.window = 0;
    disc_params.filter_policy = 0;
    disc_params.limited = 0;

    rc = ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &disc_params, blecent_gap_event, NULL);

    if (rc != 0) {
        ESP_LOGI(TAG, "ERROR initiating GAP discovery procedure; rc=%d", rc);
    }
}

int bluetooth_manager_stop_scan(void) {
    int rc;

    rc = ble_gap_disc_cancel();
    if (rc == BLE_HS_EALREADY) rc = 0;

    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to stop scan; rc=%d", rc);
        return rc;
    }

    return rc;
}

static int discover_chr_cb(
    uint16_t conn_handle,
    const struct ble_gatt_error* error,
    const struct ble_gatt_chr* chr,
    void* arg
) {
    peer_t* peer = (peer_t*)arg;
    struct peer_svc_node* svc_node = peer->disc_svc_next;

    if (error->status == 0) {
        char buffer[100];
        ESP_LOGI(TAG, "Found characteristic: %s", ble_uuid_to_str(&chr->uuid.u, buffer));
        struct peer_chr_node* node = (struct peer_chr_node*)malloc(sizeof(struct peer_chr_node));

        if (node != NULL) {
            node->chr = *chr;
            node->next = svc_node->chrs;
            svc_node->chrs = node;
        }
    } else if (error->status == BLE_HS_EDONE) {
        svc_node = svc_node->next;
        peer->disc_svc_next = svc_node;

        if (svc_node != NULL) {
            peer->disc_svc_rc = ble_gattc_disc_all_chrs(
                peer->conn_handle,
                svc_node->svc.start_handle,
                svc_node->svc.end_handle,
                discover_chr_cb,
                peer
            );
        } else {
            xSemaphoreGive(peer->discover_sem);
        }
    } else {
        ESP_LOGE(TAG, "chr discovery error: %d", error->status);
        peer->disc_svc_rc = error->status;
        xSemaphoreGive(peer->discover_sem);
    }

    return peer->disc_svc_rc;
}

static int discover_svc_cb(
    uint16_t conn_handle,
    const struct ble_gatt_error* error,
    const struct ble_gatt_svc* service,
    void* arg
) {
    peer_t* peer = (peer_t*)arg;
    assert(peer && peer->conn_handle == conn_handle);

    if (error->status == 0) {
        char buffer[100];
        ESP_LOGI(TAG, "Found service: %s", ble_uuid_to_str(&service->uuid.u, buffer));
        struct peer_svc_node* node = (struct peer_svc_node*)malloc(sizeof(struct peer_svc_node));

        if (node != NULL) {
            node->chrs = NULL;
            node->next = peer->svcs;
            node->svc = *service;
            peer->svcs = node;
        }
    } else if (error->status == BLE_HS_EDONE) {
        struct peer_svc_node* node = peer->svcs;
        peer->disc_svc_next = node;

        peer->disc_svc_rc = ble_gattc_disc_all_chrs(
            peer->conn_handle, node->svc.start_handle, node->svc.end_handle, discover_chr_cb, peer
        );
    } else {
        ESP_LOGE(TAG, "svc discovery error: %d", error->status);
        peer->disc_svc_rc = error->status;
        xSemaphoreGive(peer->discover_sem);
    }

    return peer->disc_svc_rc;
}

int bluetooth_manager_discover_all(peer_t* peer) {
    peer->discover_sem = xSemaphoreCreateBinary();
    int rc;

    rc = ble_gattc_disc_all_svcs(peer->conn_handle, discover_svc_cb, peer);

    if (rc == 0) {
        if (xSemaphoreTake(peer->discover_sem, portMAX_DELAY)) {
            rc = peer->disc_svc_rc;
            ESP_LOGI(TAG, "Discovery Complete.");
        }
    } else {
        ESP_LOGE(TAG, "Error initializing discovery: %d", rc);
    }

    vSemaphoreDelete(peer->discover_sem);
    return rc;
}