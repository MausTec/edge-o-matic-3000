
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <time.h>

#include "VERSION.h"
#include "config.h"
#include "config_defs.h"
#include "eom-hal.h"

#include "system/http_server.h"

#include "BluetoothServer.h"
#include "Console.h"
#include "Hardware.h"
#include "OrgasmControl.h"
#include "Page.h"
#include "RunningAverage.h"
#include "UpdateHelper.h"
#include "UserInterface.h"
#include "api/broadcast.h"
#include "wifi_manager.h"

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
        for (;;) {
        }
    }

    if (!UI.begin(eom_hal_get_display_ptr())) {
        printf("SSD1306 allocation failed\n");
        for (;;) {
        }
    }
}

extern "C" void app_main() {
    eom_hal_init();
    console_init();
    http_server_init();
    wifi_manager_init();

    printf("Maus-Tec presents: Edge-o-Matic 3000\n");
    printf("Version: " VERSION "\n");
    printf("EOM-HAL Version: %s\n", eom_hal_get_version());

    // Setup Hardware
    setupHardware();
    resetSD();

    // Go to the splash page:
    Page::Go(&DebugPage, false);
    eom_hal_set_encoder_brightness(Config.led_brightness);
    eom_hal_set_encoder_rgb(255, 0, 0);

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

    // I'm always one for the dramatics:
    delay(500);
    eom_hal_set_encoder_rgb(0, 255, 0);
    delay(500);
    eom_hal_set_encoder_rgb(0, 0, 255);
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
        api_broadcast_network_status();
    }

    if (millis() - lastTick > 1000 / 15) {
        lastTick = millis();

        // Update Icons
        if (wifi_manager_get_status() == WIFI_MANAGER_CONNECTED) {
            UI.drawWifiIcon(1);
        } else {
            UI.drawWifiIcon(0);
        }

        api_broadcast_readings();
    }

    Page::DoLoop();

    // Tick and see if we need to save config:
    config_enqueue_save(-1);
}