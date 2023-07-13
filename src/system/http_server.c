#include "system/http_server.h"
#include "system/websocket_handler.h"

#include "api/index.h"

#include "config.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_https_server.h"
#include "esp_log.h"

static const char* TAG = "http_server";
static httpd_handle_t _server = NULL;

static const httpd_uri_t routes[] = {{
    .uri = "/",
    .method = HTTP_GET,
    .handler = &websocket_handler,
    .user_ctx = NULL,
    .is_websocket = true,
    .handle_ws_control_frames = true,
}};

static void init_routes(httpd_handle_t server) {
    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
        httpd_uri_t route = routes[i];
        ESP_LOGI(TAG, "* Route: %s", route.uri);
        httpd_register_uri_handler(server, &route);
    }
}

static httpd_handle_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (Config.websocket_port > 0) {
        config.server_port = Config.websocket_port;
    }

    // Open/Close callbacks
    config.open_fn = websocket_open_fd;
    config.close_fn = websocket_close_fd;

    ESP_LOGI(TAG, "Starting server on port: %d", config.server_port);
    esp_err_t err = httpd_start(&server, &config);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error starting server: %s", esp_err_to_name(err));
        return NULL;
    }

    if (Config.use_ssl) {
        // this will be enabled soon, though right now the server start handles ~only~ the HTTP server
        // and expects that as a return, so there's no easy way to also configure SSL. Also, the certs
        // should be loaded off SD card, not ASM.

        // httpd_ssl_config_t ssl_config = HTTPD_SSL_CONFIG_DEFAULT();
        // ssl_config.httpd = config;

        // extern const unsigned char cacert_pem_start[] asm("_binary_servercert_pem_start");
        // extern const unsigned char cacert_pem_end[]   asm("_binary_servercert_pem_end");
        // ssl_config.cacert_pem = cacert_pem_start;
        // ssl_config.cacert_len = cacert_pem_end - cacert_pem_start;

        // extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
        // extern const unsigned char prvtkey_pem_end[]   asm("_binary_prvtkey_pem_end");
        // ssl_config.prvtkey_pem = prvtkey_pem_start;
        // ssl_config.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

        // esp_err_t ret = httpd_ssl_start(&server, &ssl_config);
        // if (ESP_OK != ret) {
        //     ESP_LOGE(TAG, "Error starting server!");
        //     return NULL;
        // }
    }

    ESP_LOGI(TAG, "Server started, registering routes.");
    init_routes(server);

    return server;
}

static void stop_webserver(httpd_handle_t server) { httpd_stop(server); }

static void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id,
                            void* event_data) {
    httpd_handle_t* server = (httpd_handle_t*)arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver...");
        *server = start_webserver();
    }
}

static void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id,
                               void* event_data) {
    httpd_handle_t* server = (httpd_handle_t*)arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver...");
        stop_webserver(*server);
        *server = NULL;
    }
}

esp_err_t http_server_init(void) {
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &_server);
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler,
                               &_server);

    api_register_all();
    return ESP_OK;
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