#include "BluetoothServer.h"
#include "esp_log.h"

#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"

static const char *TAG = "BluetoothServer";

static uint8_t own_addr_type;

static int ble_peripheral_gap_event(struct ble_gap_event *event, void *arg) {
  return 0;
}

static void ble_peripheral_task(void *params) {
  nimble_port_run();
  // The above blocks.
  nimble_port_freertos_deinit();
}

static void ble_peripheral_advertise(void) {
  struct ble_gap_adv_params adv_params;
  struct ble_hs_adv_fields fields;
  const char *name = ble_svc_gap_device_name();
  int rc;

  memset(&fields, 0, sizeof fields);

  fields.flags = BLE_HS_ADV_F_DISC_GEN | 
                 BLE_HS_ADV_F_BREDR_UNSUP;

  fields.tx_pwr_lvl_is_present = 1;
  fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

  fields.name = (uint8_t*) name;
  fields.name_len = strlen(name);
  fields.name_is_complete = 1;

  ble_uuid_any_t uuid;
  ble_uuid_init_from_buf(&uuid, SERVICE_UUID, sizeof(SERVICE_UUID));

  fields.uuids128 = (ble_uuid128_t[]) {
    uuid.u128,
  };

  fields.num_uuids128 = 1;
  fields.uuids128_is_complete = 1;

  rc = ble_gap_adv_set_fields(&fields);
  if (rc != 0) {
    ESP_LOGE(TAG, "Error setting BLE advertisement data: rc=%d", rc);
    return;
  }

  memset(&adv_params, 0, sizeof adv_params);
  adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
  
  rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_peripheral_gap_event, NULL);

  if (rc != 0) {
    ESP_LOGE(TAG, "Error starting BLE advertisement: rc=%d", rc);
    return;
  }

  ESP_LOGI(TAG, "Now advertising.");
}

static void ble_peripheral_on_reset(int reason) {
  ESP_LOGW(TAG, "BLE Reset, reason=%d", reason);
}

static void ble_peripheral_on_sync(void) {
  int rc;
  
  rc = ble_hs_util_ensure_addr(0);
  if (rc != 0) {
    ESP_LOGE(TAG, "could not... ensure address? rc=%d", rc);
    return;
  }

  rc=ble_hs_id_infer_auto(0, &own_addr_type);
  if (rc != 0) {
    ESP_LOGE(TAG, "Error determining address type: rc=%d", rc);
    return;
  }

  uint8_t addr_val[6] = {0};
  rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);

  if (rc == 0) {
    ESP_LOGI(TAG, "Device Address: %s", addr_val);
  } else {
    return;
  }

  ble_peripheral_advertise();
}

static void gatt_server_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {

}

static int gatt_server_init(void) {

}

BluetoothServer::BluetoothServer() {

}

BluetoothServer::~BluetoothServer() {
  disconnect();
}

void BluetoothServer::disconnect() {
  stopAdvertising();
  // deinit nimble
  ESP_LOGI(TAG, "BLE Disconnected.");
}

// esp_err_t bluetooth_init(void) {
void BluetoothServer::begin() {
  ESP_ERROR_CHECK(esp_nimble_hci_and_controller_init());
  nimble_port_init();

  ble_hs_cfg.reset_cb = ble_peripheral_on_reset;
  ble_hs_cfg.sync_cb = ble_peripheral_on_sync;
  ble_hs_cfg.gatts_register_cb = gatt_server_register_cb;
  ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

  ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO;
  // ble_hs_cfg.sm_mitm = 1;
  // ble_hs_cfg.sm_sc = 1;
  ble_hs_cfg.sm_sc = 0;

  // if bonding
  //ble_hs_cfg.sm_bonding = 1;
  //ble_hs_cfg.sm_our_key_dist = 1;
  //ble_hs_cfg.sm_their_key_dist = 1;

  int rc = gatt_server_init();
  if (rc != 0) {
    ESP_LOGE(TAG, "Unable to initialize GATT server: rc=%d", rc);
    return;
  }

  rc = ble_svc_gap_device_name_set(Config.bt_display_name);
  if (rc != 0) {
    ESP_LOGE(TAG, "Unable to set device name: rc=%d", rc);
    return;
  }

  // There wasn't a definition for this in the NimBLE example on ESP.
  // ble_store_config_init();

  nimble_port_freertos_init(ble_peripheral_task);
}

void BluetoothServer::advertise() {
}

void BluetoothServer::stopAdvertising() {
}

BluetoothServer BT;