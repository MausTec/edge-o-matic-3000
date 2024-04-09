#include "system/http_server.h"
#include "api/index.h"
#include "config.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_https_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "system/websocket_handler.h"
#include "util/fs.h"

#define DEFAULT_CACERT_PATH "cert.pem"
#define DEFAULT_PRVTKEY_PATH "key.pem"

static const char* TAG = "http_server";
static httpd_handle_t _server = NULL;

static const httpd_uri_t routes[] = {
    {
        .uri = "/",
        .method = HTTP_GET,
        .handler = &websocket_handler,
        .user_ctx = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = true,
    },
    {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = &websocket_handler,
        .user_ctx = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = true,
    },
    {
        .uri = "*",
        .method = HTTP_GET,
        .handler = &rest_get_handler,
        .user_ctx = NULL,
        .is_websocket = false,
        .handle_ws_control_frames = false,
    },
    {
        .uri = "*",
        .method = HTTP_POST,
        .handler = &rest_get_handler,
        .user_ctx = NULL,
        .is_websocket = false,
        .handle_ws_control_frames = false,
    },
    {
        .uri = "*",
        .method = HTTP_OPTIONS,
        .handler = &rest_get_handler,
        .user_ctx = NULL,
        .is_websocket = false,
        .handle_ws_control_frames = false,
    },
};

static void init_routes(httpd_handle_t server) {
    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
        httpd_uri_t route = routes[i];
        ESP_LOGI(TAG, "* Route: %s", route.uri);
        httpd_register_uri_handler(server, &route);
    }
}

static httpd_handle_t start_webserver(void) {
    esp_err_t err = ESP_OK;
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (Config.websocket_port > 0) {
        config.server_port = Config.websocket_port;
    }

    // Allow wildcard matching:
    config.uri_match_fn = httpd_uri_match_wildcard;

    // Open/Close callbacks
    config.open_fn = websocket_open_fd;
    config.close_fn = websocket_close_fd;

    if (!Config.use_ssl) {
        ESP_LOGI(TAG, "Starting server on port: %d", config.server_port);
        err = httpd_start(&server, &config);
    } else {
        ESP_LOGI(
            TAG,
            "Starting SSL server on port: %d (%d bytes free)",
            config.server_port,
            esp_get_free_heap_size()
        );

        httpd_ssl_config_t ssl_config = HTTPD_SSL_CONFIG_DEFAULT();
        ssl_config.httpd = config;

        char path[CONFIG_PATH_MAX];

        sniprintf(
            path,
            CONFIG_PATH_MAX,
            "%s/%s",
            fs_sd_root(),
            Config.ssl_cacert_path ? Config.ssl_cacert_path : DEFAULT_CACERT_PATH
        );

        ssl_config.cacert_len = fs_read_file(path, (char**)&ssl_config.cacert_pem);

        if (ssl_config.cacert_len <= 0) {
            ESP_LOGE(TAG, "Error reading CA Certificate.");
            return NULL;
        } else {
            ESP_LOGI(TAG, "%s", ssl_config.cacert_pem);
        }

        sniprintf(
            path,
            CONFIG_PATH_MAX,
            "%s/%s",
            fs_sd_root(),
            Config.ssl_prvtkey_path ? Config.ssl_prvtkey_path : DEFAULT_PRVTKEY_PATH
        );

        ssl_config.prvtkey_len = fs_read_file(path, (char**)&ssl_config.prvtkey_pem);

        if (ssl_config.prvtkey_len <= 0) {
            ESP_LOGE(TAG, "Error reading SSL Private Key.");
            free(ssl_config.cacert_pem);
            return NULL;
        } else {
            ESP_LOGI(TAG, "%s", ssl_config.prvtkey_pem);
        }

        ESP_LOGI(TAG, "SSL Certificates provisioned. (%d bytes free)", esp_get_free_heap_size());
        err = httpd_ssl_start(&server, &ssl_config);

        free(ssl_config.cacert_pem);
        free(ssl_config.prvtkey_pem);
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error starting server: %s", esp_err_to_name(err));
        return NULL;
    }

    ESP_LOGI(TAG, "Server started, registering routes.");
    init_routes(server);

    return server;
}

static void stop_webserver(httpd_handle_t server) {
    httpd_stop(server);
}

static void
connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    httpd_handle_t* server = (httpd_handle_t*)arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver...");
        *server = start_webserver();
    }
}

static void
disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    httpd_handle_t* server = (httpd_handle_t*)arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver...");
        stop_webserver(*server);
        *server = NULL;
    }
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
    if (_server == NULL) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

void http_server_disconnect(void) {
    stop_webserver(_server);
    _server = NULL;
}