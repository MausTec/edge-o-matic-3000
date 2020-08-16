#include <WiFi.h>
#include <WebSocketsServer.h>

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include <FastLED.h>
#include <analogWrite.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// For the Butt Device:
// MotorControl
// PressureSensor
// UserInterface
// WirelessLink
// SDCard

#define MODE_AUTO 0
#define MODE_MANUAL 1

// GPIO
#define BUTT_PIN 34
#define LED_PIN 15
#define LED_COUNT 13

#define ENCODER_R_PIN 2
#define ENCODER_G_PIN 0
#define ENCODER_B_PIN 4

#define WINDOW_SIZE 10
int RA_Index = 0;
int RA_Value = 0;
int RA_Sum = 0;
int RA_Readings[WINDOW_SIZE] = {0};
int RA_Averaged = 0;

uint8_t LED_Brightness = 13;

uint8_t mode = MODE_AUTO;

// LCD
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_DC 13
#define OLED_RESET 14
#define OLED_CS 12

Adafruit_SSD1306 display(128, 64, &SPI, OLED_DC, OLED_RESET, OLED_CS);

float arousal = 0.0;

CRGB leds[LED_COUNT];
CRGB encoderColor = CRGB::Black;

// Constants
const char* ssid = "Here Be Dragons";
const char* password = "RAWR!barkbark";

// Globals
WebSocketsServer webSocket = WebSocketsServer(80);
uint8_t last_connection;
int ramp_time_s = 30;
int motor_max = 255;
float motor_speed = 0;
uint16_t peak_limit = 600;

void sendSettings(uint8_t num) {
  StaticJsonDocument<200> doc;
  doc["cmd"] = "SETTINGS";
  doc["brightness"] = LED_Brightness;
  doc["peak_limit"] = peak_limit;
  doc["motor_max"] = motor_max;
  doc["ramp_time_s"] = ramp_time_s;

  // Blow the Network Load
  String payload;
  serializeJson(doc, payload);
  webSocket.sendTXT(num, payload);
}

void sendWxStatus() {
  StaticJsonDocument<200> doc;
  doc["cmd"] = "WIFI_STATUS";
  doc["ssid"] = ssid;
  doc["ip"] = WiFi.localIP().toString();
  doc["signal_strength"] = WiFi.RSSI();

  // Blow the Network Load
  String payload;
  serializeJson(doc, payload);
  webSocket.sendTXT(last_connection, payload);
}

void sendSdStatus() {
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
  webSocket.sendTXT(last_connection, payload);
}

void onMessage(uint8_t num, uint8_t * payload) {
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
      LED_Brightness = doc["brightness"];
    } else if (strcmp(cmd, "SET_LIMIT") == 0) {
      peak_limit = doc["limit"];
    } else if (strcmp(cmd, "GET_SETTINGS") == 0) {
      sendSettings(num);
    } else if (strcmp(cmd, "RESET_SD") == 0) {
      resetSD();
    } else if (strcmp(cmd, "GET_SD_STATUS") == 0) {
      sendSdStatus();
    } else if (strcmp(cmd, "GET_WIFI_STATUS") == 0) {
      sendWxStatus();
    } else {
      Serial.println("???");
    }
  }
}

// Called when receiving any WebSocket message
void onWebSocketEvent(uint8_t num,
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
        IPAddress ip = webSocket.remoteIP(num);
        last_connection = num;
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

void resetSD() {
  // SD
  if(!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }

  uint8_t cardType = SD.cardType();

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
      Serial.println("MMC");
  } else if(cardType == CARD_SD){
      Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
      Serial.println("SDHC");
  } else {
      Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  sendSdStatus();
}

void drawChartAxes() {
  // Y-axis
  display.drawLine(2, 10, 2, display.height() - 3, SSD1306_WHITE);
  for (int i = 0; i < display.height() - 10; i += 5) {
    display.drawLine(0, i, 2, i, SSD1306_WHITE);
  }
  
  // X-axis
  display.drawLine(2, display.height() - 2, display.width(), display.height() - 2, SSD1306_WHITE);
  for (int i = 0; i < display.width() - 2; i += 5) {
    display.drawLine(i, display.height() - 2, i, display.height(), SSD1306_WHITE);
  }
}

void drawChart() {
  display.fillRect(4, 10, display.width() - 3, display.height() - 3 - 10, SSD1306_WHITE);
}

void setup() {

  // Start Serial port
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTT_PIN, INPUT);

  pinMode(ENCODER_R_PIN, OUTPUT);
  pinMode(ENCODER_G_PIN, OUTPUT);
  pinMode(ENCODER_B_PIN, OUTPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
  } else {
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(0,0);
    display.cp437(true);
    display.write("Starting...");

    drawChartAxes();
    display.display();
  }

  resetSD();

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, LED_COUNT);
  for(int i = 0; i < LED_COUNT; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();

  // Connect to access point
  Serial.println("Connecting");
  WiFi.begin(ssid, password);
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }

  // Print our IP address
  Serial.println("Connected!");
  Serial.print("My IP address: ");
  Serial.println(WiFi.localIP());

  display.setCursor(0,0);
  display.print(WiFi.localIP().toString());
  display.print("    ");
  display.display();

  // Start WebSocket server and assign callback
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
}

void loop() {
  switch(mode) {
    case MODE_AUTO:
      encoderColor = CRGB::Green;
    case MODE_MANUAL:
      encoderColor = CRGB::Blue;
  }

  // Look for and handle WebSocket data
  webSocket.loop();

  static long lastTick = 0;
  static long lastStatusTick = 0;
  static int led_i = 0;
  static uint16_t last_value = 0;
  static uint16_t peak_start = 0;

  // Periodically send out WiFi status:
  if (millis() - lastStatusTick > 1000 * 10) {
    lastStatusTick = millis();
    sendWxStatus();
  }
  
  if (millis() - lastTick > 1000 / 30) {
    lastTick = millis();

    // Calculate Value
    RA_Sum = RA_Sum - RA_Readings[RA_Index];
    RA_Value = analogRead(BUTT_PIN);
    RA_Readings[RA_Index] = RA_Value;
    RA_Sum = RA_Sum + RA_Value;
    RA_Index = (RA_Index + 1) % WINDOW_SIZE;
    RA_Averaged = RA_Sum / WINDOW_SIZE;

    // Calculate Arousal
    if (RA_Value < last_value) {
      if (RA_Value > peak_start) {
        if (RA_Value - peak_start >= peak_limit / 10) {
          arousal += RA_Value - peak_start;
        }
      }
      peak_start = RA_Value;
    }

    // Natural Arousal Decay
    arousal *= 0.99;
    last_value = RA_Value;

    // Ramp Motor:
    // make this a signed value and use negative values to control off delay.
    float motor_increment = ((float)motor_max / (30.0 * (float)ramp_time_s));
    
    if (arousal > peak_limit) {
      motor_speed = -0.5 * (float)ramp_time_s * 30.0 * motor_increment;
    } else if (motor_speed < 255) {
      motor_speed += motor_increment;
    }

    // Update LEDs
    uint8_t bar = map(arousal, 0, peak_limit, 0, LED_COUNT - 1);
    uint8_t dot = map(RA_Averaged, 0, 4096, 0, LED_COUNT - 1);
    for (uint8_t i = 0; i < LED_COUNT; i++) {
      if (i < bar) {
        leds[i] = CRGB(map(i, 0, LED_COUNT-1, 0, LED_Brightness), map(i, 0, LED_COUNT-1, LED_Brightness, 0), 0);
      } else {
        leds[i] = CRGB::Black;
      }
    }

    leds[dot] = CRGB(LED_Brightness, 0, LED_Brightness);
    
    FastLED.show();

    analogWrite(ENCODER_R_PIN, encoderColor.r);
    analogWrite(ENCODER_G_PIN, encoderColor.g);
    analogWrite(ENCODER_B_PIN, encoderColor.b);

    // Update Chart
    drawChart();
    display.display();

    // Serialize Data
    StaticJsonDocument<200> doc;
    doc["pressure"] = RA_Value;
    doc["pavg"] = RA_Averaged;
    doc["motor"] = floor(max(motor_speed, (float)0.0));
    doc["arousal"] = arousal;
    doc["millis"] = millis();

    // Blow the Network Load
    String payload;
    serializeJson(doc, payload);
    webSocket.sendTXT(last_connection, payload);
  }
}
