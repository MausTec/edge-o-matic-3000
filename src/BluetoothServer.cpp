#include "../include/BluetoothServer.h"

BluetoothServer::BluetoothServer() {

}

BluetoothServer::~BluetoothServer() {
  disconnect();
}

void BluetoothServer::disconnect() {
  BLEDevice::deinit();
  stopAdvertising();

  free(this->characteristic);
  this->characteristic = nullptr;

  free(this->service);
  this->service = nullptr;

  free(this->server);
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
      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_WRITE
  );

  Serial.println("Set Characteristic");
  this->characteristic->setValue("What?");
  Serial.println("BLE Running.");
}

void BluetoothServer::advertise() {
  this->advertising = this->server->getAdvertising();
  this->advertising->start();
}

void BluetoothServer::stopAdvertising() {
  this->advertising->stop();
  free(this->advertising);
  this->advertising = nullptr;
}

BluetoothServer BT;