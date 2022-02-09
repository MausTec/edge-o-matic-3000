
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <time.h>

#include "eom-hal.h"
#include "config.h"
#include "config_defs.h"
#include "VERSION.h"

#include "Hardware.h"
#include "UserInterface.h"
#include "BluetoothServer.h"
#include "Page.h"
#include "RunningAverage.h"
#include "Console.h"
#include "OrgasmControl.h"
#include "WiFiHelper.h"
#include "UpdateHelper.h"
#include "WebSocketHelper.h"
#include "WebSocketSecureHelper.h"

#include "polyfill.h"

void loop(void);

UserInterface UI;

void resetSD() {
  long long int cardSize = eom_hal_get_sd_size_bytes();

  if (cardSize == -1) {
    UI.drawSdIcon(0);
    printf("Card Mount Failed\n");
    config_load_default(&Config);
    return;
  }

  UI.drawSdIcon(1);
  printf("SD Card Size: %lluMB\n", cardSize);
  config_load_from_sd(CONFIG_FILENAME, &Config);
  printf("Config loaded, testing save...\n");
  config_save_to_sd(CONFIG_FILENAME, &Config);
  printf("Save completed.\n");
}

void setupHardware() {
  // pinMode(MOT_PWM_PIN, OUTPUT);

  if (!Hardware::initialize()) {
    printf("Hardware initialization failed!\n");
    for (;;) {}
  }
  
  printf("Hardware initialized, testing save...\n");
  config_save_to_sd(CONFIG_FILENAME, &Config);
  printf("Save completed.\n");

  if (!UI.begin(eom_hal_get_display_ptr())) {
    printf("SSD1306 allocation failed\n");
    for (;;) {}
  }
  
  printf("Display initialized, testing save...\n");
  config_save_to_sd(CONFIG_FILENAME, &Config);
  printf("Save completed.\n");
}

TaskHandle_t BackgroundLoopTask;

void backgroundLoop(void*) {
  for (;;) {
    WebSocketHelper::tick();
    vTaskDelay(1);
  }
}

extern "C" void app_main() {
  eom_hal_init();

  printf("Maus-Tec presents: Edge-o-Matic 3000\n");
  printf("Version: " VERSION "\n");
  printf("EOM-HAL Version: %s\n", eom_hal_get_version());

  // Setup Hardware
  resetSD();
  setupHardware();
  

  // Go to the splash page:
  Page::Go(&DebugPage, false);
  Hardware::setEncoderColor(CRGB::Red);

  UI.drawWifiIcon(1);
  UI.render();

  // Initialize WiFi
  if (Config.wifi_on) {
    WiFiHelper::begin();
  }

  // Initialize Bluetooth
  if (Config.bt_on) {
    printf("Starting up Bluetooth...\n");
    BT.begin();
    printf("Now Discoverable!\n");
    BT.advertise();
  }


  // Start background worker:
  xTaskCreatePinnedToCore(
    backgroundLoop, /* Task function. */
    "backgroundLoop",   /* name of task. */
    10000,     /* Stack size of task */
    NULL,      /* parameter of the task */
    1,         /* priority of the task */
    &BackgroundLoopTask,    /* Task handle to keep track of created task */
    0);        /* pin task to core 0 */

// I'm always one for the dramatics:
  delay(500);
  Hardware::setEncoderColor(CRGB::Green);
  delay(500);
  Hardware::setEncoderColor(CRGB::Blue);
  delay(500);
  Hardware::setEncoderColor(CRGB::White);
  delay(500);
  UI.fadeTo();

  Page::Go(&RunGraphPage);
  printf("READY\n");

  for (;;) {
    loop();
    vTaskDelay(1);
  }
}

void loop() {
  eom_hal_tick();
  Console::loop(); // <- TODO rename to tick
  Hardware::tick();
  OrgasmControl::tick();
  UI.tick();

  static long lastStatusTick = 0;
  static long lastTick = 0;
  static int led_i = 0;

  // Periodically send out WiFi status:
  if (millis() - lastStatusTick > 1000 * 10) {
    lastStatusTick = millis();
    WebSocketHelper::sendWxStatus();
  }

  if (millis() - lastTick > 1000 / 15) {
    lastTick = millis();

    // Update LEDs
#ifdef LED_PIN
    uint8_t bar = map(OrgasmControl::getArousal(), 0, Config.sensitivity_threshold, 0, LED_COUNT - 1);
    uint8_t dot = map(OrgasmControl::getAveragePressure(), 0, 4096, 0, LED_COUNT - 1);
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
#endif

    // Update Icons
    WiFiHelper::drawSignalIcon();
    WebSocketHelper::sendReadings();
  }

  Page::DoLoop();

  // Tick and see if we need to save config:
  save_config_to_sd(-1);
}