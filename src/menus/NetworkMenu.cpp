#include "UIMenu.h"
#include "UserInterface.h"
#include "wifi_manager.h"
#include "BluetoothServer.h"
#include "BluetoothDriver.h"
#include "WebSocketHelper.h"
#include <string>

static void onDisableWiFi(UIMenu *menu) {
  UI.toastNow("Disconnecting...", 0, false);
  Config.wifi_on = false;
  wifi_manager_disconnect();
  config_enqueue_save(0);
  UI.toast("Disconnected.", 3000);
  menu->initialize();
  menu->rerender();
}

static void onEnableWiFi(UIMenu *menu) {
  Config.wifi_on = true;

  if (Config.wifi_ssid[0] == '\0' || Config.wifi_key[0] == '\0') {
    menu->rerender();
    return;
  }

  UI.toastNow("Connecting...", 0, false);
  if (wifi_manager_connect_to_ap(Config.wifi_ssid, Config.wifi_key) == ESP_OK) {
    UI.toastNow("WiFi Connected!", 3000);
    config_enqueue_save(0);
    menu->initialize();
  } else {
    UI.toastNow("Failed to connect.", 3000);
  }

  menu->rerender();
}

static void onViewStatus(UIMenu*) {
  std::string status = "";

  if (Config.bt_on) {
    status += "Bluetooth On\n";
    status += Config.bt_display_name;
    status += "\n";
  } 
  
  if (wifi_manager_get_status() == WIFI_MANAGER_CONNECTED) {
    status += "Connected\n";
    status += wifi_manager_get_local_ip() + '\n';
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

    if (wifi_manager_get_status() == WIFI_MANAGER_CONNECTED) {
      wifi_manager_disconnect();
    }
  }

  config_enqueue_save(0);
  BT.begin();

  menu->initialize();
  menu->render();
  UI.toastNow("Bluetooth On", 3000);
}

static void onDisableBluetooth(UIMenu* menu) {
  UI.toastNow("Disconnecting...", 0, false);
  Config.bt_on = false;
  config_enqueue_save(0);
  BT.disconnect();
  menu->initialize();
  menu->render();
  UI.toastNow("Disconnected.", 3000);
}

static void onDisconnect(UIMenu *menu) {
  wifi_manager_disconnect();
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
  
  if (wifi_manager_get_status() == WIFI_MANAGER_CONNECTED) {
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