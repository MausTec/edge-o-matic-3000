#include "system/http_server.h"
#include "system/websocket_handler.h"

#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"

static const char *TAG = "http_server";
static httpd_handle_t _server = NULL;

static const httpd_uri_t ws = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = &websocket_handler,
    .user_ctx = NULL,
    .is_websocket = true
};

static void routes(httpd_handle_t server) {
    httpd_register_uri_handler(server, &ws);
}

static httpd_handle_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG, "Starting server on port: %d", config.server_port);
    esp_err_t err = httpd_start(&server, &config);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Server started, registering routes.");
        routes(server);
        return server;
    }

    ESP_LOGW(TAG, "Error starting server: %s", esp_err_to_name(err));
    return NULL;
}

static void stop_webserver(httpd_handle_t server) {
    httpd_stop(server);
}

static void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver...");
        *server = start_webserver();
    }
}

static void disconnect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver...");
        stop_webserver(*server);
        *server = NULL;
    }
}

esp_err_t http_server_init(void) {
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &_server);
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &_server);
}

esp_err_t http_server_connect(void) {
    _server = start_webserver();
    if (_server == NULL) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

void http_server_disconnect(void) {
    stop_webserver(_server);
    _server = NULL;
}