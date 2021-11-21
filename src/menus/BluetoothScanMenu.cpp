#include "UserInterface.h"
#include "UIMenu.h"
#include "BluetoothDriver.h"
#include "esp_log.h"

#include <NimBLEDevice.h>

static int scanTime = 30; //In seconds
static BLEScan* pBLEScan = nullptr;
static bool scanning = false;
static UIMenu* globalMenuPtr = nullptr;

static const char* TAG = "BluetoothScanMenu";

static void startScan(UIMenu*);
static void stopScan(UIMenu*);

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    UIMenu* menuPtr = nullptr;

public:
    MyAdvertisedDeviceCallbacks(UIMenu* menu) {
        this->menuPtr = menu;
    }

    static void onConnect(UIMenu* menu, void* devicePtr) {
        BLEAdvertisedDevice* device = (BLEAdvertisedDevice*) devicePtr;
        ESP_LOGE(TAG, "Clicked Device: %s", device->getAddress().toString().c_str());

        UI.toastNow("Connecting...", -1, false);
        BluetoothDriver::Device* bt_dev = BluetoothDriver::buildDevice(device);

        if (bt_dev != nullptr) {
            BluetoothDriver::registerDevice(bt_dev);
            UI.toast("Connected!");
            UI.closeMenu();
            return;
        } else {
            UI.toastNow("Device not supported.");
        }

        stopScan(menu);
    }

    void onResult(BLEAdvertisedDevice* advertisedDevice) {
        ESP_LOGI(TAG, "Advertised Device: %s (%p)", advertisedDevice->getAddress().toString().c_str(), advertisedDevice);

        if (advertisedDevice->haveName()) {
            if (this->menuPtr->getIndexByArgument(advertisedDevice) < 0) {
                this->menuPtr->addItem(advertisedDevice->getName().c_str(), &this->onConnect, advertisedDevice);
                this->menuPtr->render();
            }
        }
    }
};

static void freeMenuPtrs(UIMenu* menu) {
    //  UIMenuItem *ptr = menu->firstItem();
    //
    //  while (ptr != nullptr) {
    //    if (ptr->arg != nullptr) {
    //      BLEAddress *addr = (BLEAddress*) ptr->arg;
    //      delete addr;
    //      ptr->arg = nullptr;
    //    }
    //    ptr = ptr->next;
    //  }
}

static void addScanItem(UIMenu* menu) {
    menu->removeItem(0);

    if (!scanning) {
        menu->addItemAt(0, "Scan for Devices", &startScan);
    } else {
        menu->addItemAt(0, "Stop Scanning", &stopScan);
    }
}

static void startScan(UIMenu* menu) {
    scanning = true;
    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
    freeMenuPtrs(menu);
    menu->initialize();
    menu->render();

    pBLEScan->start(scanTime, [](BLEScanResults foundDevices) {
        scanning = false;
        addScanItem(globalMenuPtr);
        globalMenuPtr->render();
        }, false);
}

static void stopScan(UIMenu* menu) {
    scanning = false;
    if (pBLEScan->isScanning()) {
        pBLEScan->stop();
    }
    addScanItem(menu);
    menu->render();
}

static void menuOpen(UIMenu* menu) {
    pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(menu));
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);  // less or equal setInterval value

    // Default start scan:
    startScan(menu);
}

static void menuClose(UIMenu* menu) {
    pBLEScan->stop();
    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
    scanning = false;
    freeMenuPtrs(menu);
    pBLEScan = nullptr;
}

static void buildMenu(UIMenu* menu) {
    globalMenuPtr = menu;
    menu->onOpen(&menuOpen);
    menu->onClose(&menuClose);

    addScanItem(menu);
}

UIMenu BluetoothScanMenu("Bluetooth Pair", &buildMenu);