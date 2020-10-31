#ifndef __WebSocketHelper_h
#define __WebSocketHelper_h

#include <WebSocketsServer.h>
#include <map>

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

typedef struct WebSocketConnection {
  int num;
  IPAddress ip;
  bool stream_readings = true;
  bool stream_screen_data = false;
} WebSocketConnection;

namespace WebSocketHelper {
  void begin();
  void tick();

  void send(const char *cmd, JsonDocument &doc, int num = -1);
  void send(const char *cmd, String text, int num = -1);

  void sendSettings(int num = -1);
  void sendWxStatus(int num = -1);
  void sendSdStatus(int num = -1);
  void sendReadings(int num = -1);

  namespace {
    WebSocketsServer* webSocket;
    uint8_t last_connection;
    bool stream_data = false;

    std::map<int, WebSocketConnection*> connections;

    void onMessage(int num, uint8_t * payload);

    void onWebSocketEvent(int num,
                          WStype_t type,
                          uint8_t * payload,
                          size_t length);
  }
}

#endif