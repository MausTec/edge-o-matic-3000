#include <sstream>

// Max clients to be connected to the ESP32
#define MAX_HTTPS_CLIENTS 1
#define MAX_HTTP_CLIENTS 5
#define MAX_CLIENTS (MAX_HTTP_CLIENTS + MAX_HTTPS_CLIENTS)

#include "esp_websocket_client.h"
#include "esp_https_server.h"

namespace WebSocketSecureHelper {
  void setup();
  void end();
  void loop();

  void registerRemote(esp_websocket_client_handle_t client);

  // Send message to a specific client or all clients.
  void send(int num, const char *msg);

  // Declare some handler functions for the various URLs on the server
  // The signature is always the same for those functions. They get two parameters,
  // which are pointers to the request data (read request body, headers, ...) and
  // to the response data (write response, set status code, ...)
  void handleRoot(httpd_req_t* req);
  void handle404(httpd_req_t* req);
  void handleCors(httpd_req_t* req);

  // As websockets are more complex, they need a custom class that is derived from WebsocketHandler
  class RemoteHandler {
  public:
    // This method is called by the webserver to instantiate a new handler for each
    // client that connects to the websocket endpoint
    static RemoteHandler* create();

    // This method is called when a message arrives
    void onMessage(void* input);

    // Handler function on connection close
    void onClose();

    // Gets the current index
    int getIndex();

    // Sends text
    virtual void sendText(const char* payload);


    bool isBridge() { return is_bridge; };

  protected:
    bool is_bridge = false;
  };

  class BridgeHandler : public RemoteHandler {
  public:
    BridgeHandler(esp_websocket_client_handle_t client);

    static BridgeHandler* create(esp_websocket_client_handle_t client);

    void sendText(const char* payload);

  private:
    esp_websocket_client_handle_t ws_client;
  };


  namespace {
    // SSLCert* cert = nullptr;
    // HTTPSServer* secureServer = nullptr;
    // HTTPServer* insecureServer = nullptr;

    // Simple array to store the active clients:
    RemoteHandler* activeClients[MAX_CLIENTS];
  }
}