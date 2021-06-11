#include "BluetoothServer.h"

BluetoothServer::BluetoothServer() {

}

BluetoothServer::~BluetoothServer() {
  disconnect();
}

void BluetoothServer::disconnect() {
  stopAdvertising();
  BLEDevice::deinit(true);

  this->characteristic = nullptr;
  this->service = nullptr;
  this->server = nullptr;

  Serial.println("BLE Disconnected.");
}

void BluetoothServer::begin() {
  Serial.println("BLEDevice::init");
  BLEDevice::init(Config.bt_display_name);

  Serial.println("Create server");
  this->server = BLEDevice::createServer();

  Serial.println("Create service");
  this->service = this->server->createService(SERVICE_UUID);

  Serial.println("Create Characteristic");
  this->characteristic = this->service->createCharacteristic(
      CHARACTERISTIC_UUID,
      NIMBLE_PROPERTY::READ |
      NIMBLE_PROPERTY::WRITE
  );

  Serial.println("Set Characteristic");
  this->characteristic->setValue("What?");
  Serial.println("BLE Running.");
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