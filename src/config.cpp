#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include "../config.h"

ConfigStruct Config;

/**
 * This code loads configuration into the Config struct.
 * @see config.h
 * void loadConfigFromSd();
 */
void loadConfigFromSd() {
  StaticJsonDocument<512> doc;

  if (!SD.exists(CONFIG_FILENAME)) {
    Serial.println("Couldn't find config.json on your SD card!");
  } else {
    File configFile = SD.open(CONFIG_FILENAME);
    DeserializationError e = deserializeJson(doc, configFile);

    if (e) {
      Serial.println(F("Failed to deserialize JSON, using default config!"));
      Serial.println(F("^-- This means no WiFi. Please ensure your SD card has config.json present."));
    }
  }

  // Copy WiFi Settings
  strlcpy(Config.wifi_ssid, doc["wifi_ssid"] | "", sizeof(Config.wifi_ssid));
  strlcpy(Config.wifi_key, doc["wifi_key"] | "", sizeof(Config.wifi_key));
  Config.wifi_on = doc["wifi_on"] | false;

  // Copy Bluetooth Settings
  strlcpy(Config.bt_display_name, doc["bt_display_name"] | "", sizeof(Config.bt_display_name));
  Config.bt_on = doc["bt_on"] | false;

  // Copy Network Settings
  Config.websocket_port = doc["websocket_port"] | 80;

  // Copy UI Settings
  Config.led_brightness = doc["led_brightness"] | 128;
  Config.screen_dim_seconds = doc["screen_dim_seconds"] | 10;

  // Copy Orgasm Settings
  Config.motor_max_speed = doc["motor_max_speed"] | 128;
  Config.pressure_smoothing = doc["pressure_smoothing"] | 5;
}