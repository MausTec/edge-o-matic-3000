#ifndef __WiFiHelper_h
#define __WiFiHelper_h

#include "Arduino.h"

namespace WiFiHelper {
  namespace {
    byte getWiFiStrength();
  }

  void drawSignalIcon();
}

#endif