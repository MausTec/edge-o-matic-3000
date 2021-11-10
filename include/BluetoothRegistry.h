#ifndef __buttplug_registry_h
#define __buttplug_registry_h

#include <vector>
#include <NimBLEDevice.h>
//#include <NimBLEAdvertisedDevice.h>

#define WAIT_FOR_NOTIFY_TIMEOUT_MS 10000

class BluetoothRegistry;

class BluetoothDevice {
public:
  friend class BluetoothRegistry;
  BluetoothDevice(std::string name);

  bool connect(NimBLEAdvertisedDevice*);
  bool disconnect();
  void sendRawCmd(std::string cmd);
  std::string readRaw(bool waitForNotify = false);
  std::string getName() { return this->name; };
  void onNotify(uint8_t *data, size_t length);
  bool waitForNotify();

  // Device Commands
  std::string deviceType();
  int battery();
  bool powerOff();
  int status();
  bool vibrate(uint8_t speed);

private:
  std::string name = "";
  BLERemoteService *service = nullptr;
  BLERemoteCharacteristic *txChar = nullptr;
  BLERemoteCharacteristic *rxChar = nullptr;
  BLEClient *client = nullptr;
  bool notifyPending = false;
};

class BluetoothRegistry {
public:
  void connect(BLEAdvertisedDevice *device);
  void disconnect(BLEAdvertisedDevice *device);
  void disconnect(BluetoothDevice *device);

  bool connected();
  size_t deviceCount();
  void vibrateAll(uint8_t speed);

  BluetoothDevice *getDeviceByCharacteristic(BLERemoteCharacteristic* characteristic);
  std::vector<BluetoothDevice*> getDevices() { return devices; }
  std::vector<BluetoothDevice*> *getDevicesPtr() { return &devices; }

private:
  std::vector<BluetoothDevice*> devices;
};

extern BluetoothRegistry Bluetooth;

#endif