#ifndef __WebSocketHelper_h
#define __WebSocketHelper_h

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

namespace WebSocketHelper {
  void begin();
  void end();
  void tick();

  void send(const char *cmd, JsonDocument &doc, int num = -1);
  void send(const char *cmd, String text, int num = -1);

  void sendSettings(int num = -1);
  void sendWxStatus(int num = -1);
  void sendSdStatus(int num = -1);
  void sendReadings(int num = -1);

  void connectToBridge(const char *hostname, int port);

  void onMessage(int num, const char * payload);

  namespace {
    uint8_t last_connection;
    bool stream_data = false;
  }
}

#endif