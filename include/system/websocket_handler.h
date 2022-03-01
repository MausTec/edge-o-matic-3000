#ifndef __websocket_handler_h
#define __websocket_handler_h

#include "cJSON.h"
#include "console.h"
#include "esp_err.h"
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int websocket_client_handle_t;
typedef command_err_t (*websocket_command_func_t)(cJSON* command, cJSON* response);

struct websocket_command {
    const char* command;
    websocket_command_func_t func;
};

typedef struct websocket_command websocket_command_t;

struct websocket_client {
    httpd_handle_t server;
    int fd;
};

typedef struct websocket_client websocket_client_t;

esp_err_t websocket_handler(httpd_req_t* req);
esp_err_t websocket_open_fd(httpd_handle_t hd, int sockfd);
void websocket_close_fd(httpd_handle_t hd, int sockfd);

void websocket_register_command(websocket_command_t* command);
void websocket_run_command(const char* command, cJSON* data, cJSON* response);
void websocket_run_commands(cJSON* commands, cJSON* response);

// Broadcast update triggers:
esp_err_t websocket_broadcast(cJSON* root);

#ifdef __cplusplus
}
#endif

#endif
