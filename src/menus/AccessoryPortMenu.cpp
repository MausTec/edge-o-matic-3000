#include "eom-hal.h"

#include "UIMenu.h"
#include "UserInterface.h"
#include "AccessoryDriver.h"

static int scanTime = 30; //In seconds
static bool scanning = false;

static const char *TAG = "AccessoryPortMenu";

static void selectDevice(UIMenu *menu, void *v_device) {
  AccessoryDriver::Device *d = (AccessoryDriver::Device*) v_device;
  AccessoryDriver::registerDevice(d);
  UI.toastNow("Connected.");
  return;
}

static void busItemFound(eom_hal_accessory_bus_device_t *device, void *v_menu) {
  UIMenu *menu = (UIMenu*) v_menu;
  
  // TODO: Actually read the EEPROM chip and detect TSCODE or other protocols, also remove scanning
  //       from HAL since that's a hardware-agnostic function.

  AccessoryDriver::Device *d = new AccessoryDriver::Device(
    device->display_name, 
    device->address, 
    AccessoryDriver::PROTOCOL_TSCODE
  );

  ESP_LOGD(TAG, "Found Device %02x v %02x", device->address, d->address);

  // The comparison with 0x2e here is so we can't connect to our digipot.
  menu->addItem(device->display_name, device->address == 0x2e ? nullptr : &selectDevice, d);
  menu->render();
}

static void stopScan(UIMenu *menu);

static void startScan(UIMenu *menu) {
  scanning = true;
  menu->initialize();
  menu->render();

  eom_hal_accessory_scan_bus_p(&busItemFound, (void*) menu);
  stopScan(menu);
}


static void addScanItem(UIMenu *menu) {
  menu->removeItem(0);

  if (!scanning) {
    menu->addItemAt(0, "Scan for Devices", &startScan);
  } else {
    menu->addItemAt(0, "Scanning...", nullptr);
  }
}

static void stopScan(UIMenu *menu) {
  scanning = false;
  addScanItem(menu);
  menu->render();
}

static void menuOpen(UIMenu *menu) {
  eom_hal_set_accessory_mode(EOM_HAL_ACCESSORY_MASTER);
}

static void menuClose(UIMenu *menu) {
  
}

static void buildMenu(UIMenu *menu) {
  // menu->onOpen(&menuOpen);
  menu->onClose(&menuClose);
  menu->enableAutoCleanup(AUTOCLEAN_DELETE);

  addScanItem(menu);
}

UIMenu AccessoryPortMenu("Accessory Port", &buildMenu);