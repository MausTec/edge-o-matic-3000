
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
#include "UserInterface.h"
#include "api/broadcast.h"
#include "wifi_manager.h"
#include "accessory_driver.h"

#include "polyfill.h"

void loop(void);

UserInterface UI;

static const char* TAG = "main";

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

static void orgasm_task(void *args) {
    // for (;;) {
        OrgasmControl::tick();

        vTaskDelay(1);
    // }
}

static void hal_task(void *args) {
    // for (;;) {
        eom_hal_tick();
        Hardware::tick();

        vTaskDelay(1);
    // }
}

static void ui_task(void *args) {
    // for (;;) {
        UI.tick();
        Page::DoLoop();

        vTaskDelay(1);
    // }
}

static void dump_tasks(void) {
    size_t n = uxTaskGetNumberOfTasks();
    TaskHandle_t th = NULL;
    size_t waste = 0;

    ESP_LOGI(TAG, "ID  Task Name            High W");

    for (size_t i = 0; i < n; i++) {
        th = pxTaskGetNext(th);
        if (th == NULL) break;
        size_t highw = uxTaskGetStackHighWaterMark(th);
        waste += highw;
        ESP_LOGI(TAG, "%-3d %-20s %-4d", i, pcTaskGetName(th), highw);
    }

    ESP_LOGI(TAG, "-------------------------------");
    ESP_LOGI(TAG, "Wasted memory: %d", waste);

    ESP_LOGI(TAG, "Heap used: %d/%d (%02f) (%d bytes free, max %d)\n", 
        heap_caps_get_total_size(MALLOC_CAP_8BIT) - heap_caps_get_free_size(MALLOC_CAP_8BIT), 
        heap_caps_get_total_size(MALLOC_CAP_8BIT),
        (1.0f - ((float) heap_caps_get_free_size(MALLOC_CAP_8BIT) / heap_caps_get_total_size(MALLOC_CAP_8BIT))) * 100,
        heap_caps_get_free_size(MALLOC_CAP_8BIT),
        heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)
    );
}

static void loop_task(void *args) {
    // for (;;) {
        static long lastStatusTick = 0;
        static long lastTick = 0;

        // Periodically send out WiFi status:
        if (millis() - lastStatusTick > 1000 * 10) {
            lastStatusTick = millis();
            api_broadcast_network_status();

            dump_tasks();

            // Update Icons
            if (wifi_manager_get_status() == WIFI_MANAGER_CONNECTED) {
                int8_t rssi = wifi_manager_get_rssi();
                uint8_t status = ((100 + rssi) / 10);
                UI.drawWifiIcon(status < 4 ? status : 4);
            } else {
                UI.drawWifiIcon(1);
            }
        }

        if (millis() - lastTick > 1000 / 15) {
            lastTick = millis();
            api_broadcast_readings();
        }

        // Tick and see if we need to save config:
        config_enqueue_save(-1);

        vTaskDelay(1);
    // }
}

extern "C" void app_main() {
    eom_hal_init();
    console_init();
    http_server_init();
    wifi_manager_init();
    accessory_driver_init();

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
        if (ESP_OK == wifi_manager_connect_to_ap(Config.wifi_ssid, Config.wifi_key)) {
            UI.drawWifiIcon(2);
            UI.render();
        }
    }

    // Initialize Bluetooth
    if (Config.bt_on) {
        printf("Starting up Bluetooth...\n");
        BT.begin();
        printf("Now Discoverable!\n");
        BT.advertise();
        UI.drawBTIcon(1);
    } else {
        UI.drawBTIcon(0);
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
        loop_task(NULL);
        hal_task(NULL);
        ui_task(NULL);
        orgasm_task(NULL);
    }
}