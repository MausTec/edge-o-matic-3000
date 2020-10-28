#include "../include/WebSocketHelper.h"
#include "../VERSION.h"

#include <FS.h>
#include <SD.h>
#include <SPI.h>
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

#include "../include/Console.h"
#include "../include/OrgasmControl.h"
#include "../include/Hardware.h"

#include "../config.h"

namespace WebSocketHelper {
  void begin() {
    // Start WebSocket server and assign callback
    webSocket = new WebSocketsServer(Config.websocket_port);
    webSocket->begin();
    webSocket->onEvent(onWebSocketEvent);
    Serial.println("Websocket server running.");
  }

  void tick() {
    if (webSocket != nullptr)
      webSocket->loop();
  }

  void sendSystemInfo(int num) {
    if (webSocket == nullptr) return;
    StaticJsonDocument<200> doc;

    doc["device"] = "Edge-o-Matic 3000";
    doc["serial"] = "";
    doc["hwVersion"] = "";
    doc["fwVersion"] = VERSION;

    // Blow the Network Load
    StaticJsonDocument<1024> envelope;
    envelope["info"] = doc;

    String payload;
    serializeJson(envelope, payload);
    webSocket->sendTXT(num > 0 ? num : last_connection, payload);
  }

  void sendSettings(int num) {
    if (webSocket == nullptr) return;

    StaticJsonDocument<200> doc;
    doc["cmd"] = "SETTINGS";
    doc["peak_limit"] = Config.sensitivity_threshold;
    doc["motor_max"] = Config.motor_max_speed;
    doc["ramp_time_s"] = Config.motor_ramp_time_s;

    // Blow the Network Load
    String payload;
    serializeJson(doc, payload);
    webSocket->sendTXT(num > 0 ? num : last_connection, payload);
  }

  void sendWxStatus(int num) {
    if (webSocket == nullptr) return;

    StaticJsonDocument<200> doc;
    doc["cmd"] = "WIFI_STATUS";
    doc["ssid"] = Config.wifi_ssid;
    doc["ip"] = WiFi.localIP().toString();
    doc["rssi"] = WiFi.RSSI();

    // Blow the Network Load
    String payload;
    serializeJson(doc, payload);
    webSocket->sendTXT(num > 0 ? num : last_connection, payload);
  }

  void sendSdStatus(int num) {
    if (webSocket == nullptr) return;

    uint8_t cardType = SD.cardType();
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);

    StaticJsonDocument<200> doc;
    doc["cmd"] = "SD_STATUS";
    doc["size"] = cardSize;

    switch(cardType) {
      case CARD_MMC:
        doc["type"] = "MMC";
        break;
      case CARD_SD:
        doc["type"] = "SD";
        break;
      case CARD_SDHC:
        doc["type"] = "SDHC";
        break;
      default:
        doc["type"] = "UNKNOWN";
        break;
    }

    // Blow the Network Load
    String payload;
    serializeJson(doc, payload);
    webSocket->sendTXT(num > 0 ? num : last_connection, payload);
  }

  void sendReadings(int num) {
    if (webSocket == nullptr) return;
//    String screenshot;
//    UI.screenshot(screenshot);

    // Serialize Data
    DynamicJsonDocument doc(3072);
    doc["pressure"] = OrgasmControl::getLastPressure();
    doc["pavg"] = OrgasmControl::getAveragePressure();
    doc["motor"] = Hardware::getMotorSpeed();
    doc["arousal"] = OrgasmControl::getArousal();
    doc["millis"] = millis();
//    doc["screenshot"] = screenshot;

    StaticJsonDocument<1024> envelope;
    envelope["readings"] = doc;

    // Blow the Network Load
    String payload;
    serializeJson(envelope, payload);
    webSocket->sendTXT(num > 0 ? num : last_connection, payload);
  }

  namespace {
    void onMessage(int num, uint8_t * payload) {
      Serial.printf("[%u] %s", num, payload);
      Serial.println();

      StaticJsonDocument<200> doc;
      DeserializationError err = deserializeJson(doc, payload);

      if (err) {
        Serial.println("Deserialization Error!");
      } else if(doc["cmd"]) {
        const char* cmd = doc["cmd"];
        Serial.println(cmd);

        if (strcmp(cmd, "SET_BRIGHTNESS") == 0) {
        } else if (strcmp(cmd, "SET_LIMIT") == 0) {
          Config.sensitivity_threshold = doc["limit"];
        } else if (strcmp(cmd, "GET_SETTINGS") == 0) {
          sendSettings(num);
        } else if (strcmp(cmd, "RESET_SD") == 0) {
//          resetSD();
        } else if (strcmp(cmd, "GET_SD_STATUS") == 0) {
          sendSdStatus(num);
        } else if (strcmp(cmd, "GET_WIFI_STATUS") == 0) {
          sendWxStatus(num);
        } else if (strcmp(cmd, "CONSOLE") == 0) {
          String response;
          String payload;

          DynamicJsonDocument resp_doc(3072);
          resp_doc["cmd"] = "CONSOLE_RESP";
          resp_doc["nonce"] = doc["nonce"];

          char line[SERIAL_BUFFER_LEN];
          strlcpy(line, doc["line"], SERIAL_BUFFER_LEN - 1);

          Console::handleMessage(line, response);
          resp_doc["resp"] = response;

          serializeJson(resp_doc, payload);
          webSocket->sendTXT(num, payload);
        } else if (strcmp(cmd, "STREAM") == 0) {
          stream_data = doc["enabled"];
        } else {
          Serial.println("???");
        }
      }
    }

    // Called when receiving any WebSocket message
    void onWebSocketEvent(int num,
                          WStype_t type,
                          uint8_t * payload,
                          size_t length) {

      // Figure out the type of WebSocket event
      switch(type) {

        // Client has disconnected
        case WStype_DISCONNECTED:
          Serial.printf("[%u] Disconnected!\n", num);
          break;

          // New client has connected
        case WStype_CONNECTED:
        {
          IPAddress ip = webSocket->remoteIP(num);
          last_connection = num;
          sendSystemInfo(num);
          Serial.printf("[%u] Connection from ", num);
          Serial.println(ip.toString());
        }
          break;

          // Echo text message back to client
        case WStype_TEXT:
          onMessage(num, payload);
          break;

          // For everything else: do nothing
        case WStype_BIN:
        case WStype_ERROR:
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
        default:
          break;
      }
    }
  }
}