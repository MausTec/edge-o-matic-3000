#include "BluetoothServer.h"
#include "esp_log.h"

static const char *TAG = "BluetoothServer";

BluetoothServer::BluetoothServer() {

}

BluetoothServer::~BluetoothServer() {
  disconnect();
}

void BluetoothServer::disconnect() {
  stopAdvertising();
  NimBLEDevice::deinit(true);

  this->characteristic = nullptr;
  this->service = nullptr;
  this->server = nullptr;

  ESP_LOGI(TAG, "BLE Disconnected.");
}

void BluetoothServer::begin() {
  ESP_LOGI(TAG, "BLEDevice::init");
  NimBLEDevice::init(Config.bt_display_name);

  ESP_LOGI(TAG, "Create server");
  this->server = BLEDevice::createServer();

  ESP_LOGI(TAG, "Create service");
  this->service = this->server->createService(SERVICE_UUID);

  ESP_LOGI(TAG, "Create Characteristic");
  this->characteristic = this->service->createCharacteristic(
      CHARACTERISTIC_UUID,
      NIMBLE_PROPERTY::READ |
      NIMBLE_PROPERTY::WRITE
  );

  ESP_LOGI(TAG, "Set Characteristic");
  this->characteristic->setValue("What?");
  ESP_LOGI(TAG, "BLE Running.");
}

void BluetoothServer::advertise() {
//  this->advertising = this->server->getAdvertising();
  // FIXME: Broken after move to NimBLE
//  this->advertising->start();
}

void BluetoothServer::stopAdvertising() {
  if (this->advertising != nullptr) {
    this->advertising->stop();
    free(this->advertising);
    this->advertising = nullptr;
  }
}

BluetoothServer BT;