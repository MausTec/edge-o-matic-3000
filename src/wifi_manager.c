#include "wifi_manager.h"

#include "system/http_server.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "config.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_tls.h"
#include "esp_wifi.h"
#include "mdns.h"
#include "nvs_flash.h"
#include "sntp.h"

#include "lwip/err.h"
#include "lwip/sys.h"

static const char* TAG = "wifi_manager";

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static char s_wifi_ip_addr_str[20] = "";
static wifi_manager_status_t s_wifi_status = WIFI_MANAGER_DISCONNECTED;

#define WIFI_MAX_CONNECTION_RETRY 5

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static void
event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            wifi_config_t config = { 0 };
            esp_wifi_get_config(WIFI_IF_STA, &config);

            if (config.ap.ssid[0] != '\0' && Config.wifi_on) {
                ESP_LOGI(TAG, "Auto-connect WiFi to: %s", config.ap.ssid);
                esp_wifi_connect();
            }
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            if (s_wifi_status == WIFI_MANAGER_DISCONNECTING) {
                s_wifi_status = WIFI_MANAGER_DISCONNECTED;
                return;
            } else if (s_retry_num < WIFI_MAX_CONNECTION_RETRY) {
                s_wifi_status = WIFI_MANAGER_RECONNECTING;
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGI(TAG, "WiFi Disconnected, Retrying (attempt %d)", s_retry_num);
            } else {
                s_wifi_status = WIFI_MANAGER_DISCONNECTED;
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                ESP_LOGW(TAG, "WiFi Disconnected, exceeded retry limit.");
            }
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
            snprintf(s_wifi_ip_addr_str, 20, IPSTR, IP2STR(&event->ip_info.ip));
            ESP_LOGI(TAG, "Got IP: %s", s_wifi_ip_addr_str);
            s_retry_num = 0;
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            s_wifi_status = WIFI_MANAGER_CONNECTED;
        }
    }
}

void wifi_manager_init(void) {
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_tls_init_global_ca_store());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id
    ));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip
    ));

    // initialize wifi with default config
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Use Internet Time
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

esp_err_t wifi_manager_connect_to_ap(const char* ssid, const char* key) {
    if (ssid[0] == '\0') {
        ESP_LOGW(TAG, "Connecting to AP requires SSID. Aborting.");
        return;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false,
            },
        },
    };

    strlcpy((char*)wifi_config.sta.ssid, ssid, 32);
    strlcpy((char*)wifi_config.sta.password, key, 64);

    esp_wifi_stop();
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to WiFi: \"%s\"", wifi_config.sta.ssid);
    ESP_LOGD(TAG, "    Using Password: \"%s\"", wifi_config.sta.password);

    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY
    );

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP.");
        // Set mDNS hostname
        if (Config.hostname[0] != '\0') {
            ESP_ERROR_CHECK(mdns_init());
            mdns_hostname_set(Config.hostname);
            mdns_instance_name_set(Config.bt_display_name);
        }

        if (Config.websocket_port > 0) {
            ESP_ERROR_CHECK(http_server_connect());
        }

        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Connection failure, check other logs.");
        return ESP_FAIL;
    } else {
        ESP_LOGE(TAG, "Unknown bitmask for connection.");
        return ESP_FAIL;
    }
}

/**
 * Scan for WiFi networks and populate a buffer. *count should initially contain the maximum number
 * of networks to scan. After this function returns, it will contain the number of networks found.
 */
esp_err_t wifi_manager_scan(wifi_ap_record_t* ap_info, uint16_t* count) {
    esp_err_t err = ESP_OK;
    uint16_t len = *count;
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(wifi_ap_record_t) * len);

    // Start a blocking scan:
    err = esp_wifi_scan_start(NULL, true);
    if (err != ESP_OK) {
        return err;
    }

    err = esp_wifi_scan_get_ap_records(count, ap_info);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_wifi_scan_get_ap_num(&ap_count);
    if (err != ESP_OK) {
        return err;
    }

    if (ap_count < *count) {
        *count = ap_count;
    }
    return err;
}

void wifi_manager_disconnect(void) {
    s_wifi_status = WIFI_MANAGER_DISCONNECTING;
    http_server_disconnect();
    esp_wifi_disconnect();
}

wifi_manager_status_t wifi_manager_get_status(void) {
    return s_wifi_status;
}

int8_t wifi_manager_get_rssi(void) {
    wifi_ap_record_t ap;
    esp_wifi_sta_get_ap_info(&ap);
    return ap.rssi;
}

const char* wifi_manager_get_local_ip(void) {
    return s_wifi_ip_addr_str;
}