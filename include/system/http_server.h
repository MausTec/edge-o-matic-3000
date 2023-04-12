#ifndef __http_server_h
#define __http_server_h

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t http_server_init(void);
esp_err_t http_server_connect(void);
void http_server_disconnect(void);

#ifdef __cplusplus
}
#endif

#endif
