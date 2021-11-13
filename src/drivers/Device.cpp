#include "drivers/Device.h"
#include <NimBLEClient.h>
#include "esp_log.h"

using namespace BluetoothDriver;

void Device::getName(char* buf, size_t len) {
    strlcpy(buf, this->display_name, len);
}

bool Device::setSpeed(uint8_t speed) {
    return true;
}

uint8_t Device::getBatteryLevel(void) {
    return 0;
}

void Device::disconnect(void) {
    if (this->ble_client->isConnected()) {
        this->ble_client->disconnect();
        NimBLEDevice::deleteClient(this->ble_client);
    }
}

Device* Device::detect(NimBLEAdvertisedDevice *device, NimBLEClient *client, NimBLERemoteService *service) {
    return nullptr;
}

bool Device::isConnected(void) {
    return this->ble_client->isConnected();
}

bool Device::reconnect(void) {
    return this->ble_client->connect();
}