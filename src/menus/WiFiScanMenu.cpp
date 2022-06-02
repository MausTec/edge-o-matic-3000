#include "UIMenu.h"
#include "UITextInput.h"
#include "UserInterface.h"
#include "esp_log.h"
#include "wifi_manager.h"

static const char* TAG = "WiFiScanMenu";

#include <cstring>

#define MAX_SCAN_NETWORKS CONFIG_WIFI_PROV_SCAN_MAX_ENTRIES

static bool scanning = false;
static void addScanItem(UIMenu* menu);

UITextInput WiFiKeyInput("WiFi Key", 64, [](UIMenu* ip) {
    UITextInput* input = (UITextInput*)ip;

    input->setValue("");

    input->onConfirm([](const char* key, UIMenu* menu) {
        UITextInput* inp = (UITextInput*)menu;
        char* ssid = (char*)inp->getCurrentArg();

        UI.toastNow("Connecting...", 0, false);

        Config.wifi_on = true;
        strlcpy(Config.wifi_ssid, ssid, WIFI_SSID_MAX_LEN);
        strlcpy(Config.wifi_key, key, WIFI_KEY_MAX_LEN);

        if (wifi_manager_connect_to_ap(Config.wifi_ssid, Config.wifi_key) == ESP_OK) {
            UI.toastNow("WiFi Connected!", 3000);
            config_enqueue_save(0);
            menu->close();
        } else {
            UI.toastNow("Failed to connect.", 3000);
        }
    });
});

static void selectNetwork(UIMenu* menu, void* ssid) {
    UI.openMenu(&WiFiKeyInput, true, true, ssid);
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

    wifi_ap_record_t ap_info[MAX_SCAN_NETWORKS];
    size_t count = MAX_SCAN_NETWORKS;

    esp_err_t err = wifi_manager_scan(ap_info, &count);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error scanning for WiFi networks: %s", esp_err_to_name(err));
        UI.toastNow("Error scanning for\nWiFi networks.");
    }

    for (int i = 0; i < count; i++) {
        wifi_ap_record_t network = ap_info[i];
        char* ssid = (char*)malloc(sizeof(char) * (strlen((char*)network.ssid) + 1));
        strcpy(ssid, (char*)network.ssid);
        menu->addItem((char*)network.ssid, &selectNetwork, (void*)ssid);
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
        menu->enableAutoCleanup(AutocleanMethod::AUTOCLEAN_FREE);
    } else {
        menu->addItem("WiFi Is Off", nullptr);
    }
}

UIMenu WiFiScanMenu("Scan WiFi Networks", &buildMenu);