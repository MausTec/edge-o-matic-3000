#include <sstream>

// Max clients to be connected to the ESP32
#define MAX_HTTPS_CLIENTS 1
#define MAX_HTTP_CLIENTS 5
#define MAX_CLIENTS (MAX_HTTP_CLIENTS + MAX_HTTPS_CLIENTS)

// We will use wifi
#include <WiFi.h>

// Includes for the server
#include <HTTPSServer.hpp>
#include <HTTPServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <WebsocketHandler.hpp>


// The HTTPS Server comes in a separate namespace. For easier use, include it here.
using namespace httpsserver;

namespace WebSocketSecureHelper {
  void setup();
  void end();
  void loop();

  // Send message to a specific client or all clients.
  void send(int num, String msg);

  // Declare some handler functions for the various URLs on the server
  // The signature is always the same for those functions. They get two parameters,
  // which are pointers to the request data (read request body, headers, ...) and
  // to the response data (write response, set status code, ...)
  void handleRoot(HTTPRequest *req, HTTPResponse *res);
  void handle404(HTTPRequest *req, HTTPResponse *res);
  void handleCors(HTTPRequest *req, HTTPResponse *res);

  // As websockets are more complex, they need a custom class that is derived from WebsocketHandler
  class RemoteHandler : public WebsocketHandler {
  public:
    // This method is called by the webserver to instantiate a new handler for each
    // client that connects to the websocket endpoint
    static WebsocketHandler *create();

    // This method is called when a message arrives
    void onMessage(WebsocketInputStreambuf *input);

    // Handler function on connection close
    void onClose();

    // Gets the current index
    int getIndex();

    // Sends text
    void sendText(String payload);
  };


  namespace {
    SSLCert *cert = nullptr;
    HTTPSServer *secureServer = nullptr;
    HTTPServer *insecureServer = nullptr;

    // Simple array to store the active clients:
    WebsocketHandler *activeClients[MAX_CLIENTS];
  }
}