#ifndef __websocket_handler_h
#define __websocket_handler_h

#include "cJSON.h"
#include "console.h"
#include "esp_err.h"
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

struct websocket_client {
    httpd_handle_t server;
    int fd;
    int broadcast_flags;
};

typedef struct websocket_client websocket_client_t;

typedef int websocket_client_handle_t;
typedef command_err_t (*websocket_command_func_t
)(cJSON* command, cJSON* response, websocket_client_t* client);

struct websocket_command {
    const char* command;
    websocket_command_func_t func;
};

typedef struct websocket_command websocket_command_t;

enum {
    // Broadcast sensor readings and arousal levels:
    WS_BROADCAST_READINGS = (1 << 0),

    // Broadcast system info, including periodic SD and WiFi updates:
    WS_BROADCAST_SYSTEM = (1 << 1),
};

esp_err_t rest_get_handler(httpd_req_t* req);
esp_err_t websocket_handler(httpd_req_t* req);
esp_err_t websocket_open_fd(httpd_handle_t hd, int sockfd);
void websocket_close_fd(httpd_handle_t hd, int sockfd);

void websocket_register_command(const websocket_command_t* command);
void websocket_run_command(
    const char* command, cJSON* data, cJSON* response, websocket_client_t* client
);
void websocket_run_commands(cJSON* commands, cJSON* response, websocket_client_t* client);

// Broadcast update triggers:
esp_err_t websocket_broadcast(cJSON* root, int broadcast_flags);

esp_err_t websocket_connect_to_bridge(const char* address, int port);

#ifdef __cplusplus
}
#endif

#endif
