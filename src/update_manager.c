#include "update_manager.h"
#include "VERSION.h"

#include "esp_log.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_crt_bundle.h"
#include <string.h>

#define MAX_HTTP_OUTPUT_BUFFER 4096

static const char* TAG = "update_manager";
static um_update_status_t _update_status = UM_UPDATE_NOT_CHECKED;

esp_err_t _http_update_event_handler(esp_http_client_event_t* evt) {
    return ESP_OK;
}

esp_err_t _http_event_handler(esp_http_client_event_t* evt) {
    static char* output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read

    switch (evt->event_id) {
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        if (!esp_http_client_is_chunked_response(evt->client)) {
            // If user_data buffer is configured, copy the response into the buffer
            if (evt->user_data) {
                if (evt->data_len > MAX_HTTP_OUTPUT_BUFFER) {
                    ESP_LOGE(TAG, "Data length greater than HTTP buffer.");
                    return ESP_FAIL;
                }
                memcpy(evt->user_data + output_len, evt->data, evt->data_len);
            } else {
                if (output_buffer == NULL) {
                    output_buffer = (char*) malloc(esp_http_client_get_content_length(evt->client));
                    output_len = 0;
                    if (output_buffer == NULL) {
                        ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                        return ESP_FAIL;
                    }
                }

                memcpy(output_buffer + output_len, evt->data, evt->data_len);
            }

            output_len += evt->data_len;
        }

        break;

    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        if (output_buffer != NULL) {
            // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
            // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
            free(output_buffer);
            output_buffer = NULL;
        }

        if (evt->user_data) {
            ((char*) evt->user_data)[output_len] = '\0';
        }

        output_len = 0;
        break;

    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
        if (err != 0) {
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        break;

    default:
        ESP_LOGD(TAG, "HTTP Event: %d", evt->event_id);
        break;
    }
    return ESP_OK;
}

esp_err_t update_manager_update_from_web() {
    char* buffer = (char*) malloc(MAX_HTTP_OUTPUT_BUFFER);
    esp_err_t err = ESP_OK;

    if (buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for response buffer.");
        return ESP_FAIL;
    }

    buffer[0] = '\0';

    esp_http_client_config_t data_config = {
        .url = REMOTE_UPDATE_URL,
        .user_data = buffer,
        .event_handler = _http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&data_config);

    err = esp_http_client_perform(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP GET failed: %s", esp_err_to_name(err));
        free(buffer);
        return err;
    }

    ESP_LOGI(TAG, "Got %d bytes: %s", strlen(buffer), buffer);

    cJSON* data = cJSON_Parse(buffer);
    free(buffer);

    if (!cJSON_IsObject(data)) {
        ESP_LOGE(TAG, "Server response was not a valid JSON object.");
        cJSON_Delete(data);
        return ESP_FAIL;
    }

    if (cJSON_HasObjectItem(data, "version") && cJSON_HasObjectItem(data, "date")) {
        ESP_LOGI(TAG, "Downloading asset for version %s, released: %s",
            cJSON_GetObjectItem(data, "version")->valuestring,
            cJSON_GetObjectItem(data, "date")->valuestring
        );
    } else {
        ESP_LOGW(TAG, "Possibly improperly formatted data.");
    }

    if (!cJSON_HasObjectItem(data, "assets")) {
        ESP_LOGE(TAG, "Server response did not contain Asset key.");
        cJSON_Delete(data);
        return ESP_FAIL;
    }

    cJSON* assets = cJSON_GetObjectItem(data, "assets");
    cJSON* cursor = NULL;
    char *url = NULL;

    cJSON_ArrayForEach(cursor, assets) {
        if (cJSON_HasObjectItem(cursor, "role") && cJSON_HasObjectItem(cursor, "url")) {
            char *role = cJSON_GetObjectItem(cursor, "role")->valuestring;
            ESP_LOGI(TAG, "Asset has role: %s", role);
            if (! strcmp(role, "image")) {
                url = cJSON_GetObjectItem(cursor, "url")->valuestring;
                break;
            }
        }
    }

    if (url == NULL) {
        ESP_LOGE(TAG, "Could not find an asset with an image role.");
        err = ESP_FAIL;
    } else {
        esp_http_client_config_t config = {
            .url = url,
            .event_handler = _http_update_event_handler,
            .keep_alive_enable = true,
            .skip_cert_common_name_check = true,
            .buffer_size_tx = 4096,
            .crt_bundle_attach = esp_crt_bundle_attach,
        };

        ESP_LOGI(TAG, "Downloading update from: %s", config.url);
        err = esp_https_ota(&config);
    }

    cJSON_Delete(data);
    return err;
}

esp_err_t update_manager_check_latest_version(semver_t* version) {
    char* buffer = (char*) malloc(MAX_HTTP_OUTPUT_BUFFER);

    if (buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for response buffer.");
        return ESP_FAIL;
    }

    buffer[0] = '\0';

    esp_http_client_config_t config = {
        .url = REMOTE_UPDATE_URL "/version.txt",
        .user_data = buffer,
        .event_handler = _http_event_handler,
    };

    ESP_LOGI(TAG, "Checking for OTA updates: %s", REMOTE_UPDATE_URL "/version.txt");

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP GET failed: %s", esp_err_to_name(err));
        free(buffer);
        return err;
    }

    ESP_LOGI(TAG, "Got %d bytes: %s", strlen(buffer), buffer);
    ESP_LOG_BUFFER_HEX(TAG, buffer, strlen(buffer));

    char version_str[40] = "";

    if (semver_parse(buffer + (buffer[0] == 'v' ? 1 : 0), version) == -1) {
        ESP_LOGE(TAG, "Failed to parse version string \"%s\"!", buffer);
        free(buffer);
        return ESP_FAIL;
    }

    semver_render(version, version_str);

    ESP_LOGI(TAG, "Found version: %s", version_str);
    return ESP_OK;
}

um_update_status_t update_manager_get_status(void) {
    return _update_status;
}

um_update_status_t update_manager_check_for_updates() {
    semver_t v_current = {};
    semver_t v_remote = {};

    if (semver_parse(VERSION + (VERSION[0] == 'v' ? 1 : 0), &v_current) == -1) {
        ESP_LOGE(TAG, "Could not parse current version: %s", VERSION);
        goto umcheck_end;
    }

    if (update_manager_check_latest_version(&v_remote) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to check for remote updates.");
        goto umcheck_end;
    }

    int compare = semver_compare(v_current, v_remote);

    char s_current[40] = { 0 };
    char s_remote[40] = { 0 };
    semver_render(&v_remote, s_remote);
    semver_render(&v_current, s_current);

    ESP_LOGI(TAG, "Comparing version %s <=> %s == %d", s_current, s_remote, compare);

    if (compare < 0) {
        _update_status = UM_UPDATE_AVAILABLE;
        ESP_LOGI(TAG, "An update for your software is available!");
    } else if (compare == 0) {
        _update_status = UM_UPDATE_IS_CURRENT;
    }

umcheck_end:
    semver_free(&v_current);
    semver_free(&v_remote);
    return _update_status;
}