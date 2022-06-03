#include "system/websocket_handler.h"

#include "cJSON.h"
#include "esp_log.h"
#include "util/list.h"
#include <sys/socket.h>

static const char* TAG = "websocket_handler";

static list_t _cmd_list = LIST_DEFAULT();
static list_t _client_list = LIST_DEFAULT();

bool _is_websocket(websocket_client_t* client) {
    return httpd_ws_get_fd_info(client->server, client->fd) == HTTPD_WS_CLIENT_WEBSOCKET;
}

esp_err_t websocket_send_to_client(websocket_client_t* client, const char* msg) {
    if (!_is_websocket(client)) {
        return ESP_ERR_HTTPD_INVALID_REQ;
    }

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = msg;
    ws_pkt.len = strlen(msg);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    ws_pkt.final = true;

    return httpd_ws_send_frame_async(client->server, client->fd, &ws_pkt);
}

esp_err_t websocket_broadcast(cJSON* root, int broadcast_flags) {
    char* str = cJSON_PrintUnformatted(root);

    if (!cJSON_HasObjectItem(root, "readings")) {
        ESP_LOGD(TAG, "Broadcasting: %s", str);
    }

    websocket_client_t* client = NULL;
    list_foreach(_client_list, client) {
        if (client->broadcast_flags & broadcast_flags) {
            websocket_send_to_client(client, str);
        }
    }

    cJSON_free(str);
    return ESP_OK;
}

static void _json_add_error(cJSON* root, const char* error) {
    const char* err_key = "error";
    if (root == NULL || !cJSON_IsObject(root)) {
        ESP_LOGW(TAG, "Attempt to add error to non-object: %s", error);
        return;
    }

    cJSON* err_obj = cJSON_GetObjectItem(root, err_key);

    if (err_obj == NULL) {
        cJSON_AddStringToObject(root, err_key, error);
        return;
    }

    if (!cJSON_IsArray(err_obj)) {
        cJSON* err_array = cJSON_CreateArray();
        cJSON_ReplaceItemViaPointer(root, err_obj, err_array);
        cJSON_AddItemToArray(err_array, err_obj);
    }

    cJSON* err_item = cJSON_CreateString(error);
    cJSON_AddItemToArray(err_obj, err_item);
}

void websocket_register_command(const websocket_command_t* command) {
    list_node_t* node = list_add(&_cmd_list, command);
    if (node == NULL) {
        ESP_LOGE(TAG, "Command registration failed, NO MEM!");
    } else {
        ESP_LOGD(TAG, "* Command: %s", command->command);
    }
}

void websocket_run_command(const char* command, cJSON* data, cJSON* response,
                           websocket_client_t* client) {
    ESP_LOGD(TAG, "Running command: %s", command);

    websocket_command_t* cmd = NULL;
    list_foreach(_cmd_list, cmd) {
        ESP_LOGD(TAG, "Checking %s", cmd->command);
        if (!strcmp(command, cmd->command)) {
            command_err_t err = cmd->func(data, response, client);
            if (err != CMD_OK) {
                cJSON_AddNumberToObject(response, "errno", err);
            }
            return;
        }
    }

    _json_add_error(response, "NOT IMPLEMENTED");
}

void websocket_run_commands(cJSON* commands, cJSON* response, websocket_client_t* client) {
    cJSON* el = NULL;
    char* key = NULL;

    cJSON_ArrayForEach(el, commands) {
        key = el->string;

        if (key != NULL) {
            cJSON* nonce = cJSON_GetObjectItem(el, "nonce");
            cJSON* cmd_rsp = cJSON_CreateObject();

            if (nonce != NULL) {
                cJSON_AddNumberToObject(cmd_rsp, "nonce", nonce->valueint);
            }

            websocket_run_command(key, el, cmd_rsp, client);

            if (cJSON_GetArraySize(cmd_rsp) > 0) {
                cJSON_AddItemToObject(response, key, cmd_rsp);
            } else {
                cJSON_Delete(cmd_rsp);
            }
        }
    }
}

esp_err_t websocket_open_fd(httpd_handle_t hd, int sockfd) {
    char ipstr[INET6_ADDRSTRLEN];
    struct sockaddr_in6 addr;
    socklen_t addr_size = sizeof(addr);
    if (getpeername(sockfd, (struct sockaddr*)&addr, &addr_size) < 0) {
        ipstr[0] = '\0';
    } else {
        inet_ntop(AF_INET6, &addr.sin6_addr, ipstr, sizeof(ipstr));
    }

    ESP_LOGI(TAG, "websocket_open_fd(hd: %p, sockfd: %d) => IP: %s", hd, sockfd, ipstr);
    websocket_client_t* client = malloc(sizeof(websocket_client_t));
    client->fd = sockfd;
    client->server = hd;
    client->broadcast_flags = 0;
    list_add(&_client_list, client);
    httpd_sess_set_ctx(hd, sockfd, client, NULL);
    return ESP_OK;
}

void websocket_close_fd(httpd_handle_t hd, int sockfd) {
    ESP_LOGI(TAG, "websocket_close_fd(hd: %p, sockfd: %d)", hd, sockfd);
    websocket_client_t* client = NULL;
    list_foreach(_client_list, client) {
        if (client->fd == sockfd) {
            list_remove(&_client_list, client);
            return;
        }
    }
}

esp_err_t websocket_handler(httpd_req_t* req) {
    if (req->method == HTTP_GET) {
        ESP_LOGD(TAG, "This was the handshake.");
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t* buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    // Calculate frame length:
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGD(TAG, "Got frame, length: %d", ws_pkt.len);
    if (ws_pkt.len) {
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }

        // Actually receive frame:
        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            goto cleanup;
        }

        ESP_LOGI(TAG, "Got packet type 0x%02x, data: %s", ws_pkt.type, ws_pkt.payload);
    }

    if (ws_pkt.type == HTTPD_WS_TYPE_PONG) {
        ESP_LOGI(TAG, "Received PONG message");
    } else if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {
        httpd_ws_frame_t resp_pkt;
        memset(&resp_pkt, 0, sizeof(httpd_ws_frame_t));
        resp_pkt.type = HTTPD_WS_TYPE_TEXT;
        resp_pkt.final = true;

        cJSON* command = cJSON_Parse((char*)ws_pkt.payload);
        cJSON* response = cJSON_CreateObject();

        if (command == NULL) {
            _json_add_error(response, cJSON_GetErrorPtr());
        } else {
            if (req->sess_ctx == NULL) {
                ESP_LOGE(TAG, "Request session context was NULL!");
            } else {
                websocket_run_commands(command, response, (websocket_client_t*)req->sess_ctx);
            }
        }

        if (cJSON_GetArraySize(response) > 0) {
            resp_pkt.payload = (uint8_t*)cJSON_PrintUnformatted(response);
            resp_pkt.len = strlen((char*)resp_pkt.payload);

            ESP_LOGI(TAG, "Transmitting response: %s", resp_pkt.payload);
            ret = httpd_ws_send_frame(req, &resp_pkt);
        }

        if (command != NULL)
            cJSON_Delete(command);

        cJSON_Delete(response);
        cJSON_free(resp_pkt.payload);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send frame: %s", esp_err_to_name(ret));
        }
    }

cleanup:
    if (buf != NULL)
        free(buf);
    return ret;
}

esp_err_t websocket_connect_to_bridge(const char* address, int port) { return ESP_FAIL; }