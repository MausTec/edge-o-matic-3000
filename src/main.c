
#include "accessory_driver.h"
#include "api/broadcast.h"
#include "bluetooth_driver.h"
#include "bluetooth_manager.h"
#include "config.h"
#include "config_defs.h"
#include "console.h"
#include "eom-hal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "orgasm_control.h"
#include "polyfill.h"
#include "system/action_manager.h"
#include "system/http_server.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "util/i18n.h"
#include "version.h"
#include "wifi_manager.h"
#include <esp_https_ota.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <time.h>

static const char* TAG = "main";

void storage_init() {
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
        ESP_LOGD(
            TAG,
            "%%heap=%d, min=%d, internal=%d",
            esp_get_free_heap_size(),
            esp_get_minimum_free_heap_size(),
            esp_get_free_internal_heap_size()
        );

        lastTick = millis();
        api_broadcast_readings();
    }

    // Tick and see if we need to save config:
    config_enqueue_save(-1);

    vTaskDelay(1);
    // }
}

static void accessory_driver_task(void* args) {
    while (true) {
        bluetooth_driver_tick();
        vTaskDelay(1);
    }
}

static void main_task(void* args) {
    console_ready();
    ui_open_page(&PAGE_EDGING_STATS, NULL);
    ui_reset_idle_timer();
    orgasm_control_set_output_mode(OC_MANUAL_CONTROL);

    for (;;) {
        loop_task(NULL);
        hal_task(NULL);
        ui_task(NULL);
        orgasm_task(NULL);
    }
}

esp_err_t run_boot_diagnostic(void) {
    esp_err_t err = ESP_ERR_INVALID_ARG;
    esp_ota_img_states_t ota_state = ESP_OTA_IMG_UNDEFINED;

    const esp_partition_t* configured = esp_ota_get_boot_partition();
    const esp_partition_t* running = esp_ota_get_running_partition();

    if (configured != running) {
        ESP_LOGW(
            TAG,
            "Configured OTA boot partition at offset 0x%08" PRIx32
            ", but running from offset 0x%08" PRIx32,
            configured->address,
            running->address
        );
        ESP_LOGW(
            TAG,
            "(This can happen if either the OTA boot data or preferred boot image become corrupted "
            "somehow.)"
        );
    }

    ESP_LOGD(
        TAG,
        "Running partition type %d subtype %d (offset 0x%08" PRIx32 ")",
        running->type,
        running->subtype,
        running->address
    );

    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            // if (diagnostic_is_ok) {
            ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ...");
            esp_ota_mark_app_valid_cancel_rollback();
            err = ESP_OK;
            // } else {
            // ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
            // esp_ota_mark_app_invalid_rollback_and_reboot();
            // }
        }
    }

    return err;
}

void app_main() {
    TickType_t boot_tick = xTaskGetTickCount();

    eom_hal_init();
    ui_init();
    storage_init();
    console_init();
    http_server_init();
    wifi_manager_init();
    accessory_driver_init();
    bluetooth_driver_init();
    orgasm_control_init();
    i18n_init();
    action_manager_init();

    // Red = preboot
    eom_hal_set_sensor_sensitivity(Config.sensor_sensitivity);
    eom_hal_set_encoder_brightness(Config.led_brightness);
    eom_hal_set_encoder_rgb(255, 0, 0);

    // Welcome Preamble
    printf("Maus-Tec presents: Edge-o-Matic 3000\n");
    printf("Version: %s\n", EOM_VERSION);
    printf("EOM-HAL Version: %s\n", eom_hal_get_version());

    // Post-Update Diagnostics
    esp_err_t dxerr = run_boot_diagnostic();
    if (dxerr != ESP_ERR_INVALID_ARG) {
        if (dxerr == ESP_OK) {
            ui_toast("%s\n%s", _("Update complete."), EOM_VERSION);
        }
    }

    // Go to the splash page:
    ui_open_page(&SPLASH_PAGE, NULL);

    // Green = prepare Networking
    vTaskDelayUntil(&boot_tick, 1000UL / portTICK_PERIOD_MS);
    eom_hal_set_encoder_rgb(0, 255, 0);

    // Initialize WiFi
    if (Config.wifi_on) {
        if (ESP_OK == wifi_manager_connect_to_ap(Config.wifi_ssid, Config.wifi_key)) {
            ui_set_icon(UI_ICON_WIFI, WIFI_ICON_WEAK_SIGNAL);
        } else {
            ui_set_icon(UI_ICON_WIFI, WIFI_ICON_DISCONNECTED);
        }
    }

    // Blue = prepare Bluetooth and Drivers
    vTaskDelayUntil(&boot_tick, 1000UL / portTICK_PERIOD_MS);
    eom_hal_set_encoder_rgb(0, 0, 255);

    // Initialize Bluetooth
    if (Config.bt_on) {
        bluetooth_manager_init();
        ui_set_icon(UI_ICON_BT, BT_ICON_ACTIVE);
    } else {
        ui_set_icon(UI_ICON_BT, -1);
    }

    // Initialize Action Manager
    action_manager_load_all_plugins();

    // Final delay on encoder colors.
    vTaskDelayUntil(&boot_tick, 1000UL / portTICK_PERIOD_MS);
    ui_fade_to(0x00);

    xTaskCreate(accessory_driver_task, "ACCESSORY", 1024 * 4, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(main_task, "MAIN", 1024 * 8, NULL, tskIDLE_PRIORITY + 1, NULL);
}