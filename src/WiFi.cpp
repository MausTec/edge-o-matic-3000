#include "../include/WiFiHelper.h"

#include "../include/UserInterface.h"
#include <WiFi.h>

namespace WiFiHelper {
  void drawSignalIcon() {
    UI.drawWifiIcon(getWiFiStrength());
  }

  namespace {
    byte getWiFiStrength() {
      byte wifiStrength;
      int rssi = WiFi.RSSI();

      if (rssi < -90) {
        wifiStrength = 0;
      } else if (rssi < -80) {
        wifiStrength = 1;
      } else if (rssi < -60) {
        wifiStrength = 2;
      } else if (rssi < 0) {
        wifiStrength = 3;
      } else {
        wifiStrength = 0;
      }

      return wifiStrength;
    }
  }
}