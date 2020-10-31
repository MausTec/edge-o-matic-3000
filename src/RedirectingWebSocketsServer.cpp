#include "../include/RedirectingWebSocketsServer.h"

/**
 * This is a disgusting combination of overrides and a global variable
 * required to generate the redirect on non-websocket connections while
 * still passing the proper URL into the address bar.
 */
String last_host = "";

/**
 * called if a non Websocket connection is coming in.
 * Note: can be override
 * @param client WSclient_t *  ptr to the client struct
 */
void RedirectingWebSocketsServer::handleNonWebsocketConnection(WSclient_t * client) {
  DEBUG_WEBSOCKETS("[WS-Server][%d][handleHeader] no Websocket connection close.\n", client->num);
  String response =
      "HTTP/1.1 302 Found\r\n"
      "Server: arduino-WebSocket-Server\r\n"
      "Content-Type: text/html; charset=utf-8\r\n"
      "Cache-Control: no-cache\r\n"
      "Connection: close\r\n"
      "Sec-WebSocket-Version: 13\r\n";

  response += "Location: http://nogasm-ui.maustec.io/#";
//  response += String(client->extraHeaders) + "\r\n";
  response += String(last_host) + "\r\n";
  response += "\r\n\r\n";

  client->tcp->write(response.c_str());
  clientDisconnect(client);
};

bool RedirectingWebSocketsServer::execHttpHeaderValidation(String headerName, String headerValue) {
  if (headerName.equalsIgnoreCase(WEBSOCKETS_STRING("Host"))) {
    last_host = headerValue;
  }

  return WebSocketsServer::execHttpHeaderValidation(headerName, headerValue);
}