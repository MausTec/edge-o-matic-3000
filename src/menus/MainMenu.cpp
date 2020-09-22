#include "../../include/UIMenu.h"
#include "../../include/UserInterface.h"
#include "../../include/WiFiHelper.h"

#include <WiFi.h>

UIMenu WiFiMenu("WiFi Settings", [](UIMenu *menu) {
  if (WiFiHelper::connected()) {
    menu->addItem("Disable WiFi", [](UIMenu *m) {
      UI.toastNow("Disconnecting...", 0);
      Config.wifi_on = false;
      WiFiHelper::disconnect();
      saveConfigToSd(0);
      UI.toast("Disconnected.", 3000);
      m->initialize();
      m->render();
    });

  } else {
    menu->addItem("Enable WiFi", [](UIMenu *m) {
      if (Config.wifi_ssid[0] == '\0' || Config.wifi_key[0] == '\0') {
        UI.toastNow("No WiFi Config\nEdit config.json\non SD card.", 0);
        return;
      }

      UI.toastNow("Connecting...", 0);
      Config.wifi_on = true;
      if (WiFiHelper::begin()) {
        UI.toastNow("WiFi Connected!", 3000);
        saveConfigToSd(0);
        m->initialize();
      } else {
        UI.toastNow("Failed to connect.", 3000);
        Config.wifi_on = false;
      }

      m->render();
    });
  }

  menu->addItem("Connection Status", [](UIMenu*) {
    String status = WiFiHelper::connected() ? "Connected" : "Disconnected";
    status += "\nIP: " + WiFiHelper::ip();
    status += "\nSignal: " + WiFiHelper::signalStrengthStr();
    UI.toast(status.c_str());
  });
});

UIMenu MainMenu("Main Menu", [](UIMenu *menu) {
  menu->addItem("WiFi Settings", [](UIMenu*) {
    UI.openMenu(&WiFiMenu);
  });
});
