#include "system/http_server.h"
#include "system/websocket_handler.h"

#include "api/index.h"

#include "config.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_https_server.h"
#include "esp_log.h"

struct server_handles {
    httpd_handle_t http;
    httpd_handle_t https;
};

static const char* TAG = "http_server";
static struct server_handles _server = { NULL, NULL };

static const httpd_uri_t routes[] = { {
    .uri = "/",
    .method = HTTP_GET,
    .handler = &websocket_handler,
    .user_ctx = NULL,
    .is_websocket = true,
    .handle_ws_control_frames = true,
} };

static void init_routes(struct server_handles server) {
    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
        httpd_uri_t route = routes[i];
        ESP_LOGI(TAG, "* Route: %s", route.uri);
        if (server.http != NULL) httpd_register_uri_handler(server.http, &route);
        if (server.https != NULL) httpd_register_uri_handler(server.https, &route);
    }
}

static struct server_handles start_webserver(void) {
    struct server_handles server = { NULL, NULL };
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (Config.websocket_port > 0) {
        config.server_port = Config.websocket_port;
    }

    // Open/Close callbacks
    config.open_fn = websocket_open_fd;
    config.close_fn = websocket_close_fd;

    if (!Config.use_ssl) {
        ESP_LOGI(
            TAG, "Starting server on port: %d, (ctrl: %d)", config.server_port, config.ctrl_port
        );
        esp_err_t err = httpd_start(&server.http, &config);

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error starting server: %s", esp_err_to_name(err));
            return server;
        }
    }

    if (Config.use_ssl) {
        httpd_ssl_config_t ssl_config = HTTPD_SSL_CONFIG_DEFAULT();

        ESP_LOGI(
            TAG,
            "Starting HTTPS server on port: %d, (ctrl: %d)",
            ssl_config.httpd.server_port,
            ssl_config.httpd.ctrl_port
        );

        // Open/Close callbacks
        ssl_config.httpd.max_open_sockets = 2;
        ssl_config.httpd.open_fn = websocket_open_fd;
        ssl_config.httpd.close_fn = websocket_close_fd;

        // Config.ssl_cert_filename / Config.ssl_privkey_filename

        extern const unsigned char cacert_pem_start[] asm("_binary_cert_pem_start");
        extern const unsigned char cacert_pem_end[] asm("_binary_cert_pem_end");
        ssl_config.cacert_pem = cacert_pem_start;
        ssl_config.cacert_len = cacert_pem_end - cacert_pem_start;

        extern const unsigned char prvtkey_pem_start[] asm("_binary_privkey_pem_start");
        extern const unsigned char prvtkey_pem_end[] asm("_binary_privkey_pem_end");
        ssl_config.prvtkey_pem = prvtkey_pem_start;
        ssl_config.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

        esp_err_t ret = httpd_ssl_start(&server.https, &ssl_config);
        if (ESP_OK != ret) {
            ESP_LOGE(TAG, "Error starting server: %d - %s", ret, esp_err_to_name(ret));
            return server;
        }
    }

    ESP_LOGI(TAG, "Server started, registering routes.");
    init_routes(server);

    return server;
}

static void stop_webserver(struct server_handles* server) {
    if (server->http != NULL) {
        ESP_LOGI(TAG, "Stopping webserver...");
        httpd_stop(server->http);
        server->http = NULL;
    }

    if (server->https != NULL) {
        ESP_LOGI(TAG, "Stopping HTTPS server...");
        httpd_ssl_stop(server->https);
        server->https = NULL;
    }
}

static void
connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    struct server_handles* server = (struct server_handles*)arg;

    if (server->http == NULL) {
        ESP_LOGI(TAG, "Starting webserver...");
        *server = start_webserver();
    }
}

static void
disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    struct server_handles* server = (struct server_handles*)arg;
    stop_webserver(server);
}

esp_err_t http_server_init(void) {
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &_server);
    esp_event_handler_register(
        WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &_server
    );

    api_register_all();
    return ESP_OK;
}

esp_err_t http_server_connect(void) {
    _server = start_webserver();
    if (_server.http == NULL && _server.https == NULL) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

void http_server_disconnect(void) {
    stop_webserver(&_server);
}