#include <WiFi.h>
#include <WebSocketsServer.h>

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

#include "config.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include <FastLED.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneButton.h>
#include <ESP32Encoder.h>

#include "include/Hardware.h"
#include "include/UserInterface.h"
#include "include/BluetoothServer.h"
#include "include/Page.h"
#include "include/RunningAverage.h"

// For the Butt Device:
// MotorControl
// PressureSensor
// UserInterface
// WirelessLink
// SDCard

#define MODE_AUTO 0
#define MODE_MANUAL 1

#define WINDOW_SIZE 5

uint8_t LED_Brightness = 13;

uint8_t mode = MODE_AUTO;


Adafruit_SSD1306 display(128, 64, &SPI, OLED_DC, OLED_RESET, OLED_CS);
UserInterface UI(&display);

BluetoothServer BT;
RunningAverage PressureAverage;
float arousal = 0.0;

// Globals
WebSocketsServer* webSocket; // This is now a pointer, because apparently there is no default constructor and
                             // it *must* be initialized with a port, which we don't know until config load.
uint8_t last_connection;
int ramp_time_s = 30;
int motor_max = 255;
float motor_speed = 0;
uint16_t peak_limit = 600;

void sendSettings(uint8_t num) {
  if (webSocket == nullptr) return;

  StaticJsonDocument<200> doc;
  doc["cmd"] = "SETTINGS";
  doc["brightness"] = LED_Brightness;
  doc["peak_limit"] = peak_limit;
  doc["motor_max"] = motor_max;
  doc["ramp_time_s"] = ramp_time_s;

  // Blow the Network Load
  String payload;
  serializeJson(doc, payload);
  webSocket->sendTXT(num, payload);
}

void sendWxStatus() {
  if (webSocket == nullptr) return;

  StaticJsonDocument<200> doc;
  doc["cmd"] = "WIFI_STATUS";
  doc["ssid"] = Config.wifi_ssid;
  doc["ip"] = WiFi.localIP().toString();
  doc["signal_strength"] = WiFi.RSSI();

  // Blow the Network Load
  String payload;
  serializeJson(doc, payload);
  webSocket->sendTXT(last_connection, payload);
}

void sendSdStatus() {
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
  webSocket->sendTXT(last_connection, payload);
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
        IPAddress ip = webSocket->remoteIP(num);
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

void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void resetSD() {
  // SD
  if(!SD.begin()) {
    UI.drawSdIcon(0);
    Serial.println("Card Mount Failed");
    return;
  }

  uint8_t cardType = SD.cardType();

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
      Serial.println("MMC");
      UI.drawSdIcon(1);
  } else if(cardType == CARD_SD){
      Serial.println("SDSC");
      UI.drawSdIcon(1);
  } else if(cardType == CARD_SDHC){
      Serial.println("SDHC");
      UI.drawSdIcon(1);
  } else {
      Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  loadConfigFromSd();
}

void setupHardware() {
  pinMode(BUTT_PIN, INPUT);

  if(!Hardware::initialize()) {
    Serial.println("Hardware initialization failed!");
    for(;;){}
  }

  if(!UI.begin()) {
    Serial.println("SSD1306 allocation failed");
    for(;;){}
  }
}

void setup() {
  // Start Serial port
  Serial.begin(115200);

  // Setup Hardware
  setupHardware();

  // Go to the splash page:
  Page::Go(&DebugPage, false);

  pinMode(MOT_PWM_PIN, OUTPUT);

  // Setup SD, which loads our config
  resetSD();

  UI.drawWifiIcon(0);
  UI.render();

  // Initialize WiFi
  if (Config.wifi_on) {
    // Connect to access point
    Serial.println("Connecting");
    WiFi.begin(Config.wifi_ssid, Config.wifi_key);
    while ( WiFi.status() != WL_CONNECTED ) {
      delay(500);
      Serial.print(".");
    }

    // Print our IP address
    Serial.println("Connected!");
    Serial.print("My IP address: ");
    Serial.println(WiFi.localIP());

    // Start WebSocket server and assign callback
    webSocket = new WebSocketsServer(Config.websocket_port);
    webSocket->begin();
    webSocket->onEvent(onWebSocketEvent);
  }

  // Initialize Bluetooth
  if (Config.bt_on) {
    Serial.println("Starting up Bluetooth...");
    BT.begin();
    Serial.println("Now Discoverable!");
    BT.advertise();
  }

  Hardware::setEncoderColor(CRGB::White);

  // I'm always one for the dramatics:
  delay(3000);
  UI.fadeTo();
  Page::Go(&RunGraphPage);
}

void loop() {
  Hardware::tick();

  // Look for and handle WebSocket data
  if (webSocket != nullptr)
    webSocket->loop();

  static long lastTick = 0;
  static long lastStatusTick = 0;
  static int led_i = 0;
  static uint16_t last_value = 0;
  static uint16_t peak_start = 0;

  long pressure_value;

  // Periodically send out WiFi status:
  if (millis() - lastStatusTick > 1000 * 10) {
    lastStatusTick = millis();
    sendWxStatus();
  }
  
  if (millis() - lastTick > 1000 / 30) {
    lastTick = millis();

    // Calculate Value
    pressure_value = analogRead(BUTT_PIN);
    PressureAverage.addValue(pressure_value);
    long RA_Averaged = PressureAverage.getAverage();

    // Calculate Arousal
    if (RA_Averaged < last_value) {
      if (RA_Averaged > peak_start) {
        if (RA_Averaged - peak_start >= peak_limit / 10) {
          arousal += RA_Averaged - peak_start;
        }
      }
      peak_start = RA_Averaged;
    }

    // Natural Arousal Decay
    arousal *= 0.99;
    last_value = RA_Averaged;

    // Ramp Motor:
    // make this a signed value and use negative values to control off delay.
    float motor_increment = ((float)motor_max / (30.0 * (float)ramp_time_s));

    if (mode == MODE_AUTO) {
      if (arousal > peak_limit) {
        motor_speed = min(-255.0, -0.5f * (float) ramp_time_s * 30.0 * motor_increment);
        UI.drawStatus("STOPPED!");
      } else if (motor_speed < 255) {
        if (motor_speed > 0) {
          UI.drawStatus("Automatic");
        }
        motor_speed += motor_increment;
      } else {
        UI.drawStatus("Automatic");
      }
    } else if (mode == MODE_MANUAL) {
      UI.drawStatus("Manual");
    }

    // Update LEDs
    if (false) {
      uint8_t bar = map(arousal, 0, peak_limit, 0, LED_COUNT - 1);
      uint8_t dot = map(RA_Averaged, 0, 4096, 0, LED_COUNT - 1);
      for (uint8_t i = 0; i < LED_COUNT; i++) {
        if (i < bar) {
          Hardware::setLedColor(i, CRGB(map(i, 0, LED_COUNT - 1, 0, LED_Brightness),
                                        map(i, 0, LED_COUNT - 1, LED_Brightness, 0), 0));
        } else {
          Hardware::setLedColor(i);
        }
      }

      Hardware::setLedColor(dot, CRGB(LED_Brightness, 0, LED_Brightness));
      Hardware::ledShow();
    }

    // Update Counts
    char status[7] = "";
    float motor_int = floor(max(motor_speed, (float)0.0));
    uint8_t motor = (motor_int / 255) * 100;
    uint8_t stat_a = ((float)arousal / peak_limit) * 100;

    UI.display->setCursor(0, 10);
    sprintf(status, "M:%03d%%", motor);
    UI.display->print(status);

    UI.display->setCursor(SCREEN_WIDTH / 3, 10);
    sprintf(status, "P:%04d", RA_Averaged);
    UI.display->print(status);

    UI.display->setCursor(SCREEN_WIDTH / 3 * 2, 10);
    sprintf(status, "A:%03d%%", stat_a);
    UI.display->print(status);

    // Update Icons
    uint8_t wifiStrength;
    int rssi = WiFi.RSSI();
    if (rssi < -90) {
      wifiStrength = 0;
    } else if (rssi < -80) {
      wifiStrength = 1;
    } else if (rssi < -60) {
      wifiStrength = 2;
    } else if (rssi < 0) {
      wifiStrength = 3;
    } else {
      wifiStrength = 0;
    }

    UI.drawWifiIcon(wifiStrength);

    // Update Chart
//    UI.setMotorSpeed(motor);
    UI.addChartReading(0, RA_Averaged);
    UI.addChartReading(1, arousal);
//    UI.drawChart(peak_limit);
    UI.render();

    // Output Motor Speed
    if (motor_speed <= 0) {
      digitalWrite(MOT_PWM_PIN, LOW);
    } else if (motor_speed >= 255) {
      digitalWrite(MOT_PWM_PIN, HIGH);
    } else {
      analogWrite(MOT_PWM_PIN, motor_speed);
    }

    if (webSocket != nullptr) {
      // Serialize Data
      StaticJsonDocument<200> doc;
      doc["pressure"] = pressure_value;
      doc["pavg"] = RA_Averaged;
      doc["motor"] = (int)motor_int;
      doc["arousal"] = arousal;
      doc["millis"] = millis();

      // Blow the Network Load
      String payload;
      serializeJson(doc, payload);
      webSocket->sendTXT(last_connection, payload);
    }
  }

  Page::DoLoop();
}

// Entrypoints:

void app_main() {
  // esp32 entry
  setup();
  while(1) loop();
}

int main(void) {
  // cpp entry
  setup();
  while(1) loop();
}