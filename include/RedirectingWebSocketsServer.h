#include <WebSocketsServer.h>

class RedirectingWebSocketsServer : public WebSocketsServer {
  using WebSocketsServer::WebSocketsServer;

  void handleNonWebsocketConnection(WSclient_t *client) override;
  bool execHttpHeaderValidation(String headerName, String headerValue) override;
};