#include "ButtplugRegistry.h"
#include "UserInterface.h"

#include <NimBLEDevice.h>
#include <Arduino.h>

#include <algorithm>

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  ButtplugDevice *device = Buttplug.getDeviceByCharacteristic(pBLERemoteCharacteristic);

  if (device != nullptr) {
    device->onNotify(pData, length);
  } else {
    log_w("Notify came for unknown device.");
  }
}

bool ButtplugDevice::disconnect() {
  if (this->client != nullptr) {
    this->client->disconnect();
    NimBLEDevice::deleteClient(this->client);
  }

  auto *vec = Buttplug.getDevicesPtr();
  log_d("Removing device from registry. %d devices known.", vec->size());
  vec->erase(std::remove(vec->begin(), vec->end(), this), vec->end());
  log_d("Removed? device from registry. %d devices known.", vec->size());
  return true;
}

bool ButtplugDevice::connect(BLEAdvertisedDevice *device) {
  log_i("Connecting to: %s", device->getAddress().toString().c_str());

  try {
    this->client = BLEDevice::createClient();
    log_d("Created client.");
  } catch (...) {
    log_e("Could not create client.");
    this->disconnect();
    return false;
  }

  this->client->connect(device);
  log_d("Connected to client.");


  BLEUUID serviceUUID = device->getServiceUUID();
  if (serviceUUID.toString() == "") {
    log_w("No serviceUUID advertised.");
    this->disconnect();
    return false;
  }

  log_w("Trying advertised service: %s", serviceUUID.toString().c_str());
  this->service = this->client->getService(serviceUUID);

  if (this->service == nullptr) {
    log_w("Failed to find service: %s", serviceUUID.toString().c_str());
    this->disconnect();
    return false;
  }

  std::vector<BLERemoteCharacteristic*> *chars = this->service->getCharacteristics(true);

  // Find the chars we want:
  for (auto *c : *chars) {
    log_i("Found characteristic: %s", c->toString().c_str());

    if (c->canNotify()) {
      log_i("Found notify characteristic.");
      this->rxChar = c;
    }

    if (c->canWrite()) {
      log_i("Found write characteristic.");
      this->txChar = c;
    }
  }

  if (this->txChar == nullptr || this->rxChar == nullptr) {
    log_w("Lovense requires RX and TX characteristics to be found.");
    this->disconnect();
    return false;
  }

  this->rxChar->registerForNotify(&notifyCallback);

  log_i("Found service!");
  return true;
}

ButtplugDevice::ButtplugDevice(std::string name) {
  this->name = name;
}

void ButtplugDevice::onNotify(uint8_t *data, size_t length) {
  log_i("Got %d bytes.", length);
  this->notifyPending = true;
}

void ButtplugDevice::sendRawCmd(std::string cmd) {
  if (! this->txChar->writeValue(cmd)) {
    log_i("WRITE FAILED, DISCONNECT??");
    this->disconnect();
  }
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
  ButtplugDevice *d = new ButtplugDevice(device->getName());
  log_i("Name was: %s", d->getName().c_str());

  if (d->connect(device)) {
    devices.push_back(d);
    d->sendRawCmd("DeviceType;");
    std::string resp = d->readRaw();
    log_i("Got DeviceType: %s", resp.c_str());
    UI.toastNow("Connected!");
  } else {
    UI.toastNow("Error Connecting.");
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

bool ButtplugRegistry::connected() {
  return this->devices.size() > 0;
}

void ButtplugRegistry::vibrateAll(uint8_t speed) {
  for (auto *d : devices) {
    d->vibrate(speed);
  }
}

ButtplugRegistry Buttplug = ButtplugRegistry();