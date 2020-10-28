#ifndef __WebSocketHelper_h
#define __WebSocketHelper_h

#include <WebSocketsServer.h>


namespace WebSocketHelper {
  void begin();
  void tick();

  void sendSettings(int num = -1);
  void sendWxStatus(int num = -1);
  void sendSdStatus(int num = -1);
  void sendReadings(int num = -1);

  namespace {
    WebSocketsServer* webSocket; // This is now a pointer, because apparently there is no default constructor and
    // it *must* be initialized with a port, which we don't know until config load.
    uint8_t last_connection;
    bool stream_data = false;

    void onMessage(int num, uint8_t * payload);

    void onWebSocketEvent(int num,
                          WStype_t type,
                          uint8_t * payload,
                          size_t length);
  }
}

#endif