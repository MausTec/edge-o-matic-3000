#ifndef __buttplug_registry_h
#define __buttplug_registry_h

#include <vector>
#include <BLEAdvertisedDevice.h>

#define WAIT_FOR_NOTIFY_TIMEOUT_MS 10000

class ButtplugRegistry;

class ButtplugDevice {
public:
  friend class ButtplugRegistry;
  ButtplugDevice(BLEAdvertisedDevice *device) : device(device) {};

  bool connect();
  bool disconnect();
  void sendRawCmd(std::string cmd);
  std::string readRaw(bool waitForNotify = false);
  void onNotify(uint8_t *data, size_t length);
  bool waitForNotify();

  // Device Commands
  std::string deviceType();
  int battery();
  bool powerOff();
  int status();
  bool vibrate(uint8_t speed);

private:
  BLEAdvertisedDevice *device = nullptr;
  BLERemoteService *service = nullptr;
  BLERemoteCharacteristic *txChar = nullptr;
  BLERemoteCharacteristic *rxChar = nullptr;
  BLEClient *client = nullptr;
  bool notifyPending = false;
};

class ButtplugRegistry {
public:
  void connect(BLEAdvertisedDevice *device);
  void disconnect(BLEAdvertisedDevice *device);
  void disconnect(ButtplugDevice *device);

  bool connected();
  size_t deviceCount();
  void vibrateAll(uint8_t speed);

  ButtplugDevice *getDeviceByCharacteristic(BLERemoteCharacteristic* characteristic);

private:
  std::vector<ButtplugDevice*> devices;
};

extern ButtplugRegistry Buttplug;

#endif