#include "UIMenu.h"
#include "UserInterface.h"
#include "WiFiHelper.h"
#include "BluetoothServer.h"
#include "BluetoothDriver.h"
#include "WebSocketHelper.h"
#include <string>

// #include <WiFi.h>

static void onDisableWiFi(UIMenu *menu) {
  UI.toastNow("Disconnecting...", 0, false);
  Config.wifi_on = false;
  WiFiHelper::disconnect();
  save_config_to_sd(0);
  UI.toast("Disconnected.", 3000);
  menu->initialize();
  menu->render();
}

static void onEnableWiFi(UIMenu *menu) {
  if (Config.wifi_ssid[0] == '\0' || Config.wifi_key[0] == '\0') {
    Config.wifi_on = true;
    save_config_to_sd(0);
    menu->render();
    return;
  }

  UI.toastNow("Connecting...", 0, false);
  Config.wifi_on = true;
  if (WiFiHelper::begin()) {
    UI.toastNow("WiFi Connected!", 3000);
    save_config_to_sd(0);
    menu->initialize();
  } else {
    UI.toastNow("Failed to connect.", 3000);
    Config.wifi_on = false;
  }

  menu->render();
}

static void onViewStatus(UIMenu*) {
  std::string status = "";

  if (Config.bt_on) {
    status += "Bluetooth On\n";
    status += Config.bt_display_name;
    status += "\n";
  } 
  
  if (WiFiHelper::connected()) {
    status += "Connected\n";
    status += WiFiHelper::ip() + '\n';
  } else if (!Config.bt_on) {
    status += "Disconnected";
  }

  UI.toast(status.c_str(), 0);
}

static void onEnableBluetooth(UIMenu* menu) {
  UI.toastNow("Enabling...", 0, false);
  Config.bt_on = true;

  if (!Config.force_bt_coex) {
    Config.wifi_on = false;

    if (WiFiHelper::connected()) {
      WiFiHelper::disconnect();
    }
  }

  save_config_to_sd(0);
  BT.begin();

  menu->initialize();
  menu->render();
  UI.toastNow("Bluetooth On", 3000);
}

static void onDisableBluetooth(UIMenu* menu) {
  UI.toastNow("Disconnecting...", 0, false);
  Config.bt_on = false;
  save_config_to_sd(0);
  BT.disconnect();
  menu->initialize();
  menu->render();
  UI.toastNow("Disconnected.", 3000);
}

static void onDisconnect(UIMenu *menu) {
  WiFiHelper::disconnect();
  menu->initialize();
  menu->render();
}

static void onMausLink(UIMenu *menu) {
  WebSocketHelper::connectToBridge("192.168.1.3", 8080);
}

static void buildMenu(UIMenu *menu) {
  if (Config.bt_on) {
    menu->addItem("Disable Bluetooth", &onDisableBluetooth);
    menu->addItem(&BluetoothScanMenu);

    if (BluetoothDriver::getDeviceCount() > 0) {
      menu->addItem(&BluetoothDevicesMenu);
    }
  } else if (Config.force_bt_coex || !Config.wifi_on) {
    menu->addItem("Enable Bluetooth", &onEnableBluetooth);
  }

  if (Config.wifi_on) {
    menu->addItem("Turn WiFi Off", &onDisableWiFi);
  }
  
  if (WiFiHelper::connected()) {
    menu->addItem("Disconnect WiFi", &onDisconnect);

    menu->addItem("Maus-Link Connect", &onMausLink);
  } else if (Config.force_bt_coex || !Config.bt_on) {
    if (Config.wifi_on) {
      if (strlen(Config.wifi_ssid) > 0) {
        std::string title = "Connect To ";
        title.append(Config.wifi_ssid);
        menu->addItem(title, &onEnableWiFi);
      }
      menu->addItem(&WiFiScanMenu);
    } else {
      menu->addItem("Enable WiFi", &onEnableWiFi);
    }
  }

  menu->addItem("Connection Status", &onViewStatus);
  menu->addItem(&AccessoryPortMenu);
}

UIMenu NetworkMenu("Network Settings", &buildMenu);