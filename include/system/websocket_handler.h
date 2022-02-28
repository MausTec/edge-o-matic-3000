#ifndef __websocket_handler_h
#define __websocket_handler_h

#include "esp_err.h"
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t websocket_handler(httpd_req_t *req);

#ifdef __cplusplus
}
#endif

#endif
