#include "UIMenu.h"
#include "UITextInput.h"
#include "WiFiHelper.h"
#include <WiFi.h>
#include "UserInterface.h"

static bool scanning = false;

static void addScanItem(UIMenu* menu);

UITextInput WiFiKeyInput("WiFi Key", 64, [](UIMenu *ip) {
    UITextInput *input = (UITextInput*) ip;

    input->setValue("");

    input->onConfirm([](const char *key, UIMenu *menu) {
        UITextInput *inp = (UITextInput*) menu;
        int *idx_ptr = (int*) inp->getCurrentArg();

        const char *ssid = WiFi.SSID(*idx_ptr).c_str();
        printf("Connect to: SSID=%s, Key=%s\n", ssid, key);
        UI.toastNow("Connecting...", 0, false);

        Config.wifi_on = true;
        strlcpy(Config.wifi_ssid, ssid, WIFI_SSID_MAX_LEN);
        strlcpy(Config.wifi_key, key, WIFI_KEY_MAX_LEN);
        
        if (WiFiHelper::begin()) {
            UI.toastNow("WiFi Connected!", 3000);
            saveConfigToSd(0);
            menu->close();
        } else {
            UI.toastNow("Failed to connect.", 3000);
        }
    });
});

static void selectNetwork(UIMenu *menu, int idx) {
    int *idx_ptr = (int*) malloc(sizeof(int));
    *idx_ptr = idx;
    UI.openMenu(&WiFiKeyInput, true, true, idx_ptr);
}

static void stopScan(UIMenu* menu) {
    scanning = false;
    menu->initialize();
    menu->render();
}

static void startScan(UIMenu* menu) {
    scanning = true;
    menu->initialize();
    menu->render();

    int count = WiFi.scanNetworks();
    for (int i = 0; i < count; i++) {
        menu->addItem(WiFi.SSID(i).c_str(), &selectNetwork, i);
    }

    scanning = false;
    addScanItem(menu);
    menu->render();
}

static void addScanItem(UIMenu* menu) {
    menu->removeItem(0);

    if (!scanning) {
        menu->addItemAt(0, "Scan for Networks", &startScan);
    } else {
        menu->addItemAt(0, "Stop Scanning", &stopScan);
    }
}

static void buildMenu(UIMenu* menu) {
    if (Config.wifi_on) {
        addScanItem(menu);
    } else {
        menu->addItem("WiFi Is Off", nullptr);
    }
}

UIMenu WiFiScanMenu("Scan WiFi Networks", &buildMenu);