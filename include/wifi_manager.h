#ifndef __wifi_manager_h
#define __wifi_manager_h

#include "esp_err.h"
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

enum wifi_manager_status {
    WIFI_MANAGER_DISCONNECTED,
    WIFI_MANAGER_CONNECTED,
};

typedef enum wifi_manager_status wifi_manager_status_t;

void wifi_manager_init(void);
esp_err_t wifi_manager_connect_to_ap(const char *ssid, const char *key);
void wifi_manager_disconnect(void);
wifi_manager_status_t wifi_manager_get_status(void);
esp_err_t wifi_manager_scan(wifi_ap_record_t* ap_info, size_t *count);
const char *wifi_manager_get_local_ip(void); 

#ifdef __cplusplus
}
#endif

#endif
