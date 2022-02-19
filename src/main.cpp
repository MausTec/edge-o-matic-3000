
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
#include "wifi_manager.h"
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
  printf("SD Card Size: %llu MB\n", cardSize / 1000000ULL);
  
  config_init();
}

void setupHardware() {
  // pinMode(MOT_PWM_PIN, OUTPUT);

  if (!Hardware::initialize()) {
    printf("Hardware initialization failed!\n");
    for (;;) {}
  }

  if (!UI.begin(eom_hal_get_display_ptr())) {
    printf("SSD1306 allocation failed\n");
    for (;;) {}
  }
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
  console_init();
  wifi_manager_init();

  printf("Maus-Tec presents: Edge-o-Matic 3000\n");
  printf("Version: " VERSION "\n");
  printf("EOM-HAL Version: %s\n", eom_hal_get_version());

  // Setup Hardware
  setupHardware();
  resetSD();

  // Go to the splash page:
  Page::Go(&DebugPage, false);
  Hardware::setEncoderColor(CRGB::Red);

  UI.drawWifiIcon(1);
  UI.render();

  // Initialize WiFi
  if (Config.wifi_on) {
    wifi_manager_connect_to_ap(Config.wifi_ssid, Config.wifi_key);
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
  console_ready();

  for (;;) {
    loop();
    vTaskDelay(1);
  }
}

void loop() {
  eom_hal_tick();
  Hardware::tick();
  OrgasmControl::tick();
  UI.tick();

  static long lastStatusTick = 0;
  static long lastTick = 0;

  // Periodically send out WiFi status:
  if (millis() - lastStatusTick > 1000 * 10) {
    lastStatusTick = millis();
    WebSocketHelper::sendWxStatus();
  }

  if (millis() - lastTick > 1000 / 15) {
    lastTick = millis();

    // Update Icons
    if (wifi_manager_get_status() == WIFI_MANAGER_CONNECTED) {
      UI.drawWifiIcon(1);
    } else {
      UI.drawWifiIcon(0);
    }
    
    WebSocketHelper::sendReadings();
  }

  Page::DoLoop();

  // Tick and see if we need to save config:
  config_enqueue_save(-1);
}