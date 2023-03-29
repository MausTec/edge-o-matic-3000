
#include "accessory_driver.h"
#include "api/broadcast.h"
#include "bluetooth_manager.h"
#include "config.h"
#include "config_defs.h"
#include "console.h"
#include "eom-hal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "orgasm_control.h"
#include "polyfill.h"
#include "system/http_server.h"
#include "ui/ui.h"
#include "util/i18n.h"
#include "version.h"
#include "wifi_manager.h"
#include <time.h>

void loop(void);

static const char* TAG = "main";

void resetSD() {
    long long int cardSize = eom_hal_get_sd_size_bytes();

    if (cardSize == -1) {
        ui_set_icon(UI_ICON_SD, -1);
        config_load_default(&Config);
        return;
    }

    ui_set_icon(UI_ICON_SD, SD_ICON_PRESENT);
    printf("SD Card Size: %llu MB\n", cardSize / 1000000ULL);

    config_init();
}

static void orgasm_task(void* args) {
    // for (;;) {
    orgasm_control_tick();

    // vTaskDelay(1);
    // }
}

static void hal_task(void* args) {
    // for (;;) {
    eom_hal_tick();

    // vTaskDelay(1);
    // }
}

static void ui_task(void* args) {
    // for (;;) {
    ui_tick();
    // UI.tick();
    // Page::DoLoop();

    // vTaskDelay(1);
    // }
}

static void loop_task(void* args) {
    // for (;;) {
    static long lastStatusTick = 0;
    static long lastTick = 0;

    // Periodically send out WiFi status:
    if (millis() - lastStatusTick > 1000 * 10) {
        lastStatusTick = millis();
        api_broadcast_network_status();

        // Update Icons
        if (wifi_manager_get_status() == WIFI_MANAGER_CONNECTED) {
            int8_t rssi = wifi_manager_get_rssi();
            ui_set_icon(UI_ICON_WIFI, WIFI_ICON_STRONG_SIGNAL);
        } else {
            ui_set_icon(UI_ICON_WIFI, WIFI_ICON_DISCONNECTED);
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

static void main_task(void* args) {
    console_ready();
    ui_open_page(&PAGE_EDGING_STATS, NULL);

    for (;;) {
        loop_task(NULL);
        hal_task(NULL);
        ui_task(NULL);
        orgasm_task(NULL);
    }
}

void app_main() {
    eom_hal_init();
    ui_init();
    console_init();
    resetSD();
    http_server_init();
    wifi_manager_init();
    accessory_driver_init();
    orgasm_control_init();
    i18n_init();

    printf("Maus-Tec presents: Edge-o-Matic 3000\n");
    printf("Version: %s\n", VERSION);
    printf("EOM-HAL Version: %s\n", eom_hal_get_version());

    // Setup Hardware

    // Go to the splash page:
    // Page::Go(&DebugPage, false);

    eom_hal_set_sensor_sensitivity(Config.sensor_sensitivity);
    eom_hal_set_encoder_brightness(Config.led_brightness);
    eom_hal_set_encoder_rgb(255, 0, 0);

    // Initialize WiFi
    if (Config.wifi_on) {
        if (ESP_OK == wifi_manager_connect_to_ap(Config.wifi_ssid, Config.wifi_key)) {
            ui_set_icon(UI_ICON_WIFI, WIFI_ICON_WEAK_SIGNAL);
        } else {
            ui_set_icon(UI_ICON_WIFI, WIFI_ICON_DISCONNECTED);
        }
    }

    // Initialize Bluetooth
    if (Config.bt_on) {
        bluetooth_manager_init();
        ui_set_icon(UI_ICON_BT, BT_ICON_ACTIVE);
    } else {
        ui_set_icon(UI_ICON_BT, -1);
    }

    xTaskCreate(main_task, "MAIN", 1024 * 8, NULL, tskIDLE_PRIORITY, NULL);
}