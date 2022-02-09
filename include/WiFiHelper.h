#ifndef __WiFiHelper_h
#define __WiFiHelper_h

#include <stddef.h>
#include <stdint.h>

#define CONNECTION_TIMEOUT_S 5

namespace WiFiHelper {
  namespace {
    uint8_t getWiFiStrength();
  }

  bool begin();
  bool connected();
  void disconnect();
  const char* ip();
  const char* signalStrengthStr();
  void drawSignalIcon();
}

#endif