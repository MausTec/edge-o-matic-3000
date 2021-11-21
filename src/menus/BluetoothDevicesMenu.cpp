#include "UIMenu.h"
#include "BluetoothDriver.h"
#include "UserInterface.h"
#include "OrgasmControl.h"

static void doDisconnect(UIMenu *menu, void *d) {
  BluetoothDriver::Device *device = (BluetoothDriver::Device*) d;
  
  if (device == nullptr) {
    log_i("BLAAAAAH");
  } else {
    UI.toastNow("Disconnecting...");
    log_i("Disconnecting from device...");

    device->disconnect();
    BluetoothDriver::unregisterDevice(device);
    UI.toastNow("Disconnected.");
    UI.closeMenu();
  }
}

static void setVibrateMode(UIMenu *menu, int m) {
  // VibrationMode mode = (VibrationMode) m;

  // Serial.print("Setting mode to: ");
  // switch(mode) {
  //   case VibrationMode::Depletion:
  //     Serial.println("Depletion");
  //     break;
  //   case VibrationMode::Enhancement:
  //     Serial.println("Enhancement");
  //     break;
  //   case VibrationMode::RampStop:
  //     Serial.println("RampStop");
  //     break;
  // }
}

static void buildVibrateModeMenu(UIMenu *menu) {
  menu->addItem("Depletion", &setVibrateMode, (int) VibrationMode::Depletion);
  menu->addItem("Enhancement", &setVibrateMode, (int) VibrationMode::Enhancement);
  menu->addItem("Ramp-Stop", &setVibrateMode, (int) VibrationMode::RampStop);
}

UIMenu VibrateModeMenu("Vibrate Mode", &buildVibrateModeMenu);

static void buildDeviceMenu(UIMenu *menu) {
  BluetoothDriver::Device *device = (BluetoothDriver::Device*) menu->getCurrentArg();
  char buf[40 + 1] = "";
  device->getName(buf, 40);

  menu->setTitle(buf);
  menu->addItem("Disconnect", &doDisconnect, device);
  // menu->addItem(&VibrateModeMenu, device);
}

UIMenu ManageDeviceMenu("Manage Device", &buildDeviceMenu);

static void buildMenu(UIMenu *menu) {
  for (size_t i = BluetoothDriver::getDeviceCount(); i > 0; --i) {
    BluetoothDriver::Device *ptr = BluetoothDriver::getDevice(i);

    char buf[40 + 1] = "";
    ptr->getName(buf, 40);
    menu->addItem(buf, &ManageDeviceMenu, ptr);
  }
}

UIMenu BluetoothDevicesMenu("Bluetooth Devices", &buildMenu);