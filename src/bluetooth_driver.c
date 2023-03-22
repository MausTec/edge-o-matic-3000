#include "bluetooth_driver.h"
#include "config.h"
#include "console/console.h"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"

static const char* TAG = "bluetooth-driver";
static bluetooth_driver_scan_callback_t _scan_callback;
static void* _scan_arg;

static void blecent_on_reset(int reason) {
    ESP_LOGE(TAG, "Resetting state; reason=%d\n", reason);
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

void bluetooth_driver_init(void) {
    esp_err_t err;

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

void bluetooth_driver_deinit(void) {
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

void bluetooth_driver_start_advertising(void) {
}

static void blecent_on_disc_complete(const struct peer* peer, int status, void* arg) {

    if (status != 0) {
        /* Service discovery failed.  Terminate the connection. */
        ESP_LOGI(
            TAG,
            "Error: Service discovery failed; status=%d "
            "conn_handle=%d\n",
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
        "conn_handle=%d\n",
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

        if (fields.name_is_complete) {
            ESP_LOGI(TAG, "BLE_GAP_EVENT_DISC: %.*s", fields.name_len, (char*)fields.name);
            if (_scan_callback != NULL) {

                peer_t peer = {
                    .fields = &fields,
                    .conn_handle = 0x00,
                };

                strncpy(peer.name, (char*)fields.name, fields.name_len);
                _scan_callback(&peer, _scan_arg);
            }
        }

        // Do something with this event...
        return 0;

    case BLE_GAP_EVENT_CONNECT:
        /* A new connection was established or a connection attempt failed. */
        if (event->connect.status != 0) {
            /* Connection attempt failed; resume scanning. */
            ESP_LOGI(TAG, "ERROR: Connection failed; status=%d\n", event->connect.status);
            // blecent_scan();
        } else {
            /* Connection successfully established. */
            ESP_LOGI(TAG, "Connection established ");

            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == 0);
            // print_conn_desc(&desc);
            ESP_LOGI(TAG, "\n");

            /* Remember peer. */
            // rc = peer_add(event->connect.conn_handle);
            if (rc != 0) {
                ESP_LOGE(TAG, "Failed to add peer; rc=%d\n", rc);
                return 0;
            }
        }

        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        /* Connection terminated. */
        ESP_LOGI(TAG, "disconnect; reason=%d ", event->disconnect.reason);
        // print_conn_desc(&event->disconnect.conn);
        ESP_LOGI(TAG, "\n");

        /* Forget about peer. */
        // peer_delete(event->disconnect.conn.conn_handle);

        /* Resume scanning. */
        // blecent_scan();
        return 0;

    case BLE_GAP_EVENT_DISC_COMPLETE:
        ESP_LOGI(TAG, "discovery complete; reason=%d\n", event->disc_complete.reason);
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
            ESP_LOGI(TAG, "Failed to discover services; rc=%d\n", rc);
            return 0;
        }
#endif
        return 0;

    case BLE_GAP_EVENT_NOTIFY_RX:
        /* Peer sent us a notification or indication. */
        ESP_LOGI(
            TAG,
            "received %s; conn_handle=%d attr_handle=%d "
            "attr_len=%d\n",
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
            "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
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

void bluetooth_driver_start_scan(bluetooth_driver_scan_callback_t on_result, void* arg) {
    uint8_t own_addr_type;
    struct ble_gap_disc_params disc_params;
    int rc;

    _scan_callback = on_result;
    _scan_arg = arg;

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);

    if (rc != 0) {
        ESP_LOGI(TAG, "TAG determining address type; rc=%d\n", rc);
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
        ESP_LOGI(TAG, "ERROR initiating GAP discovery procedure; rc=%d\n", rc);
    }
}