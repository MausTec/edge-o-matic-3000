#include "UIMenu.h"
#include "ButtplugRegistry.h"
#include "UserInterface.h"
#include "OrgasmControl.h"

#include <vector>

static void doDisconnect(UIMenu *menu, void *d) {
  ButtplugDevice *device = (ButtplugDevice*) d;
  if (device == nullptr) {
    log_i("BLAAAAAH");
  } else {
    UI.toastNow("Disconnecting...");
    log_i("Disconnecting from: %s", device->getName().c_str());

    if (device->disconnect()) {
      UI.toastNow("Disconnected.");
      UI.closeMenu();
    }
  }
}

static void setVibrateMode(UIMenu *menu, int m) {
  ButtplugDevice *device = (ButtplugDevice*) menu->getCurrentArg();
  VibrationMode mode = (VibrationMode) m;

  Serial.print("Setting mode to: ");
  switch(mode) {
    case VibrationMode::Depletion:
      Serial.println("Depletion");
      break;
    case VibrationMode::Enhancement:
      Serial.println("Enhancement");
      break;
    case VibrationMode::RampStop:
      Serial.println("RampStop");
      break;
  }
}

static void buildVibrateModeMenu(UIMenu *menu) {
  ButtplugDevice *device = (ButtplugDevice*) menu->getCurrentArg();

  menu->addItem("Depletion", &setVibrateMode, (int) VibrationMode::Depletion);
  menu->addItem("Enhancement", &setVibrateMode, (int) VibrationMode::Enhancement);
  menu->addItem("Ramp-Stop", &setVibrateMode, (int) VibrationMode::RampStop);
}

UIMenu VibrateModeMenu("Vibrate Mode", &buildVibrateModeMenu);

static void buildDeviceMenu(UIMenu *menu) {
  ButtplugDevice *device = (ButtplugDevice*) menu->getCurrentArg();
  menu->setTitle(device->getName());
  menu->addItem("Disconnect", &doDisconnect, device);
  menu->addItem(&VibrateModeMenu, device);
}

UIMenu ManageDeviceMenu("Manage Device", &buildDeviceMenu);

static void buildMenu(UIMenu *menu) {
  auto vec = Buttplug.getDevices();
  log_i("Building menu from device registry. %d devices known.", vec.size());

  for (auto *d : vec) {
    menu->addItem(d->getName(), &ManageDeviceMenu, d);
  }
}

UIMenu BluetoothDevicesMenu("Bluetooth Devices", &buildMenu);