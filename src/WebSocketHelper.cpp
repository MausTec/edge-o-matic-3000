#include "WebSocketHelper.h"
#include "VERSION.h"

#include "eom-hal.h"

#include "Console.h"
#include "OrgasmControl.h"
#include "Hardware.h"
#include "Page.h"
#include "SDHelper.h"

#include "esp_websocket_client.h"

#include "config.h"
#include "polyfill.h"

static const char* TAG = "WebSocketHelper";

namespace WebSocketHelper {
  static void _ws_client_evt_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
    esp_websocket_event_data_t* data = (esp_websocket_event_data_t*) event_data;
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
      ESP_LOGE(TAG, "WEBSOCKET_EVENT_CONNECTED");
      break;
    case WEBSOCKET_EVENT_DISCONNECTED:
      ESP_LOGE(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
      break;
    case WEBSOCKET_EVENT_DATA:
      ESP_LOGE(TAG, "WEBSOCKET_EVENT_DATA");
      ESP_LOGE(TAG, "Received opcode=%d", data->op_code);
      if (data->op_code == 0x08 && data->data_len == 2) {
        ESP_LOGW(TAG, "Received closed message with code=%d", 256 * data->data_ptr[0] + data->data_ptr[1]);
      } else {
        ESP_LOGW(TAG, "Received=%.*s", data->data_len, (char*) data->data_ptr);
      }
      ESP_LOGW(TAG, "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);
      break;
    case WEBSOCKET_EVENT_ERROR:
      ESP_LOGE(TAG, "WEBSOCKET_EVENT_ERROR");
      break;
    }
  }

  void connectToBridge(const char* hostname, int port) {
    char uri[101] = "";
    const char* token = "TEST123";
    snprintf(uri, 100, "ws://%s:%d/device/%s", hostname, port, token);
    ESP_LOGE(TAG, "Connecting to Maus-Link Bridge at %s\n", uri);

    esp_websocket_client_config_t ws_cfg;
    ws_cfg.uri = uri;
    ws_cfg.port = port;

    ESP_LOGE(TAG, "init start");
    esp_websocket_client_handle_t ws_client = esp_websocket_client_init(&ws_cfg);
    ESP_LOGE(TAG, "init END");

    if (ws_client == NULL) {
      ESP_LOGE(TAG, "Failed to get ws_client.");
      return;
    }

    esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_ANY, _ws_client_evt_handler, (void*) ws_client);
    esp_err_t err = esp_websocket_client_start(ws_client);

    if (err == ESP_OK) {
      // WebSocketSecureHelper::registerRemote(ws_client);
    }
  }

  /*
   * Helpers here for parsing and responding to commands sent
   * by the client. First parameter should also be int num.
   */

  void cbSerialCmd(int num, cJSON* args) {
    // int nonce = args["nonce"];
    // String text;

    // char line[SERIAL_BUFFER_LEN];
    // strlcpy(line, args["cmd"], SERIAL_BUFFER_LEN - 1);
    // Console::handleMessage(line, text);

    // DynamicJsonDocument resp(1024);
    // resp["nonce"] = nonce;
    // resp["text"] = text;

    // send("serialCmd", resp, num);
  }

  void cbDir(int num, cJSON* args) {
    // int nonce = args["nonce"];
    // String path = args["path"];

    // if (path[0] != '/') {
    //   path = String("/") + path;
    // }

    // DynamicJsonDocument resp(1024);
    // resp["nonce"] = nonce;
    // JsonArray files = resp.createNestedArray("files");

    // File f = SD.open(path);
    // if (!f) {
    //   resp["error"] = "Invalid directory.";
    // } else {
    //   while (true) {
    //     File entry = f.openNextFile();
    //     if (!entry) {
    //       break;
    //     }

    //     JsonObject file = files.createNestedObject();
    //     file["name"] = String(entry.name());
    //     file["size"] = entry.size();
    //     file["dir"] = entry.isDirectory();

    //     entry.close();
    //   }
    //   f.close();
    // }

    // send("dir", resp, num);
  }

  void cbMkdir(int num, cJSON* args) {
    // int nonce = args["nonce"];
    // String path = args["path"];

    // DynamicJsonDocument resp(1024);
    // resp["nonce"] = nonce;
    // resp["path"] = path;


    // if (!success) {
    //   resp["error"] = "Failed to create directory.";
    // }

    // send("mkdir", resp, num);
  }

  void cbConfigSet(int num, cJSON* args) {
    // auto config = args.as<JsonObject>();
    // bool restart_required = false;

    // for (auto kvp : config) {
    //   set_config_value(kvp.key().c_str(), kvp.value().as<String>().c_str(), &restart_required);
    // }

    // // Send new settings to client:
    // config_enqueue_save(millis() + 300);
  }

  void cbSetMode(int num, cJSON* mode) {
    // RunGraphPage.setMode(mode);
  }

  void cbSetMotor(int num, cJSON* speed) {
    // Hardware::setMotorSpeed(speed);
  }

  void cbOrgasm(int num, cJSON* seconds) {
    // OrgasmControl::permitOrgasmNow(seconds);
  }

  void cbLock(int num, cJSON* value) {
    OrgasmControl::lockMenuNow(value);
  }

  void onMessage(int num, const char* payload) {
    // DynamicJsonDocument doc(1024);
    // DeserializationError err = deserializeJson(doc, payload);

    // if (err) {
    //   Serial.println("Deserialization Error!");
    // } else {
    //   for (auto kvp : doc.as<JsonObject>()) {
    //     auto cmd = kvp.key().c_str();

    //     if (!strcmp(cmd, "configSet")) {
    //       cbConfigSet(num, kvp.value());
    //     } else if (!strcmp(cmd, "info")) {
    //       sendSystemInfo(num);
    //     } else if (!strcmp(cmd, "configList")) {
    //       sendSettings(num);
    //     } else if (!strcmp(cmd, "serialCmd")) {
    //       cbSerialCmd(num, kvp.value());
    //     } else if (!strcmp(cmd, "getWiFiStatus")) {
    //       sendWxStatus(num);
    //     } else if (!strcmp(cmd, "getSDStatus")) {
    //       sendSdStatus(num);
    //     } else if (!strcmp(cmd, "setMode")) {
    //       cbSetMode(num, kvp.value());
    //     } else if (!strcmp(cmd, "setMotor")) {
    //       cbSetMotor(num, kvp.value());
    //     } else if (!strcmp(cmd, "streamReadings")) {
    //       send("error", "E_DEPRECATED");
    //     } else if (!strcmp(cmd, "dir")) {
    //       cbDir(num, kvp.value());
    //     } else if (!strcmp(cmd, "mkdir")) {
    //       cbMkdir(num, kvp.value());
    //     } else if (!strcmp(cmd, "orgasm")) {
    //       cbOrgasm(num, kvp.value());
    //     } else if (!strcmp(cmd, "lock")) {
    //       cbLock(num, kvp.value());
    //     } else {
    //       send("error", String("Unknown command: ") + String(cmd), num);
    //     }
    //   }
    // }
  }
}