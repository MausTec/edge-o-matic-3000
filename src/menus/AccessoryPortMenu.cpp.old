#include "eom-hal.h"

#include "UIMenu.h"
#include "UserInterface.h"
#include "maus_bus.h"

static int scanTime = 30; //In seconds
static bool scanning = false;

static const char *TAG = "AccessoryPortMenu";

static void selectDevice(UIMenu *menu, void *ptr) {
  maus_bus_device_t *device = maus_bus_get_scan_item_by_address((maus_bus_address_t) ptr);
  if (device == NULL) {
    UI.toastNow("Device lost!");
    return;
  }

  maus_bus_err_t err = maus_bus_register_device((maus_bus_address_t) ptr);

    if (err == MAUS_BUS_NOT_SUPPORTED) {
        UI.toastNow("Not Supported");
        return;
    }

  if (err != MAUS_BUS_OK) {
    ESP_LOGE(TAG, "Failed to register device: %d", err);
    UI.toastNow("Error Registering Device");
    return;
  }

  UI.toastNow(device->product_name);
  return;
}

static void scan_item_cb(maus_bus_device_t *device, maus_bus_address_t address, void *v_menu) {
  UIMenu *menu = (UIMenu*) v_menu;
  char path[20] = "";
  maus_bus_addr2str(path, 20, address);
  ESP_LOGI(TAG, "Found device: %s", path);
  menu->addItem(device->product_name, &selectDevice, address);
  menu->render();
}

static void startScan(UIMenu *menu);

static void addScanItem(UIMenu *menu) {
  menu->removeItem(0);

  if (!scanning) {
    menu->addItemAt(0, "Scan for Devices", &startScan);
  } else {
    menu->addItemAt(0, "Scanning...", nullptr);
  }
}

static void startScan(UIMenu *menu) {
  scanning = true;
  menu->initialize();
  menu->render();

  maus_bus_scan_bus_full(&scan_item_cb, (void*) menu);

  scanning = false;
  addScanItem(menu);
  menu->render();
}

static void menuOpen(UIMenu *menu) {
//   eom_hal_set_accessory_mode(EOM_HAL_ACCESSORY_MASTER);
}

static void menuClose(UIMenu *menu) {
  maus_bus_free_device_scan();
}

static void buildMenu(UIMenu *menu) {
//   menu->onOpen(&menuOpen);
  menu->onClose(&menuClose);
  menu->enableAutoCleanup(AUTOCLEAN_FREE);

  addScanItem(menu);
}

UIMenu AccessoryPortMenu("Accessory Port", &buildMenu);