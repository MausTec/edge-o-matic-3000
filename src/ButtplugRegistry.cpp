#include "../include/ButtplugRegistry.h"
#include "../include/UserInterface.h"

#include <BLEDevice.h>
#include <Arduino.h>

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  ButtplugDevice *device = Buttplug.getDeviceByCharacteristic(pBLERemoteCharacteristic);

  if (device != nullptr) {
    device->onNotify(pData, length);
  } else {
    log_w("Notify came for unknown device.");
  }
}

bool ButtplugDevice::connect() {
  log_i("Connecting to: %s", this->device->getAddress().toString().c_str());

  this->client = BLEDevice::createClient();
  log_d("Created client.");

  this->client->connect(this->device);
  log_d("Connected to client.");

  BLEUUID serviceUUID = this->device->getServiceUUID();
  log_w("Trying advertised service: %s", serviceUUID.toString().c_str());
  this->service = this->client->getService(serviceUUID);

  if (this->service == nullptr) {
    log_w("Failed to find service: %s", serviceUUID.toString().c_str());
    this->client->disconnect();
    return false;
  }

  std::map<std::string, BLERemoteCharacteristic*>* chars = this->service->getCharacteristics();

  // Find the chars we want:
  for (auto const &c : *chars) {
    if (c.second->canNotify()) {
      log_i("Found characteristic: %s", c.first.c_str());
      this->rxChar = c.second;
    }

    if (c.second->canWrite()) {
      log_i("Found characteristic: %s", c.first.c_str());
      this->txChar = c.second;
    }
  }

  if (this->txChar == nullptr || this->rxChar == nullptr) {
    log_w("Lovense requires RX and TX characteristics to be found.");
    this->client->disconnect();
    return false;
  }

  this->rxChar->registerForNotify(&notifyCallback);

  log_i("Found service!");
  return true;
}

void ButtplugDevice::onNotify(uint8_t *data, size_t length) {
  log_i("Got %d bytes.", length);
  this->notifyPending = true;
}

void ButtplugDevice::sendRawCmd(std::string cmd) {
  this->txChar->writeValue(cmd);
}

std::string ButtplugDevice::readRaw(bool waitForNotify) {
  if (waitForNotify) {
    this->waitForNotify();
  }

  return this->rxChar->readValue();
}

bool ButtplugDevice::waitForNotify() {
  long start_ms = millis();

  while (millis() - start_ms < WAIT_FOR_NOTIFY_TIMEOUT_MS && !notifyPending) {
    delay(10);
  }

  bool result = notifyPending;
  log_i("Notify: %s", result ? "OK" : "TIMEOUT");
  notifyPending = false;
  return result;
}

bool ButtplugDevice::vibrate(uint8_t speed) {
  char cmd[20] = "";
  int cmdArg = max(0, min((int) map(speed, 0, 255, 0, 20), 20));

  sprintf(cmd, "Vibrate:%d;", cmdArg);
  log_i("Send cmd: %s", cmd);

  this->sendRawCmd(cmd);
  return this->waitForNotify();
}

void ButtplugRegistry::connect(BLEAdvertisedDevice *device) {
  UI.toastNow("Connecting...", 0, false);
  ButtplugDevice *d = new ButtplugDevice(device);

  if (d->connect()) {
    devices.push_back(d);
    d->sendRawCmd("DeviceType;");
    std::string resp = d->readRaw();
    log_i("Got DeviceType: %s", resp.c_str());
    UI.toastNow("Connected!");
  } else {
    delete d;
  }
}

ButtplugDevice *ButtplugRegistry::getDeviceByCharacteristic(BLERemoteCharacteristic *characteristic) {
  uint16_t handle = characteristic->getHandle();

  for (auto *d : devices) {
    if (d->rxChar->getHandle() == handle || d->txChar->getHandle() == handle) {
      return d;
    }
  }

  return nullptr;
}

void ButtplugRegistry::vibrateAll(uint8_t speed) {
  for (auto *d : devices) {
    d->vibrate(speed);
  }
}

ButtplugRegistry Buttplug = ButtplugRegistry();