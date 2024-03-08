#include "config.h"
#include "config_defs.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "config";

config_t Config = { ._filename = "", ._version = 0 };

CONFIG_DEFS {
    // WiFi Settings
    CFG_STRING(wifi_ssid, "");
    CFG_STRING(wifi_key, "");
    CFG_BOOL(wifi_on, false);

    // Bluetooth Settings
    CFG_STRING(bt_display_name, "Edge-o-Matic 3000");
    CFG_BOOL(bt_on, false);
    CFG_BOOL(force_bt_coex, false);

    // Console Settings
    CFG_BOOL(store_command_history, true);
    CFG_BOOL(console_basic_mode, false);

    // Network Settings
    CFG_NUMBER(websocket_port, 80);
    CFG_BOOL(classic_serial, false);
    CFG_BOOL(use_ssl, false);
    CFG_STRING(hostname, "eom3k");

    // UI Settings
    CFG_NUMBER(led_brightness, 128);
    CFG_NUMBER(screen_dim_seconds, 0);
    CFG_NUMBER(screen_timeout_seconds, 0);
    CFG_BOOL(enable_screensaver, false);
    CFG_STRING(language_file_name, "");
    CFG_BOOL(reverse_menu_scroll, false);

    // Orgasm Settings
    CFG_NUMBER(motor_max_speed, 128);
    CFG_NUMBER(motor_start_speed, 10);
    CFG_NUMBER(edge_delay, 1000);
    CFG_NUMBER(max_additional_delay, 1000);
    CFG_NUMBER(minimum_on_time, 1000);
    CFG_NUMBER(pressure_smoothing, 5);
    CFG_NUMBER(sensitivity_threshold, 600);
    CFG_NUMBER(motor_ramp_time_s, 30);
    CFG_NUMBER(update_frequency_hz, 50);
    CFG_NUMBER(sensor_sensitivity, 128);
    CFG_BOOL(use_average_values, false);
    CFG_NUMBER(denials_count_to_orgasm, 10);
    CFG_NUMBER(milk_o_matic_rest_duration_seconds, 60);
    CFG_ENUM(to_orgasm_mode, to_orgasm_mode_t, Timer);
    CFG_BOOL(random_orgasm_triggers, false);

    // Vibration Settings
    CFG_ENUM(vibration_mode, vibration_mode_t, RampStop);

    // Post-Orgasm Torture
    CFG_BOOL(use_orgasm_modes, false);
    CFG_NUMBER(clench_pressure_sensitivity, 200);
    CFG_NUMBER(clench_time_to_orgasm_ms, 1500);
    CFG_NUMBER(clench_time_threshold_ms, 900);
    CFG_BOOL(clench_detector_in_edging, false);
    CFG_NUMBER(auto_edging_duration_minutes, 30);
    CFG_NUMBER(post_orgasm_duration_seconds, 10);
    CFG_BOOL(post_orgasm_menu_lock, false);
    CFG_BOOL(edge_menu_lock, false);
    CFG_NUMBER(max_clench_duration_ms, 3000);

    // Internal system things, only edit if you know what you're doing.
    CFG_STRING_PTR(remote_update_url, REMOTE_UPDATE_URL)

    // Please just leave this here and don't ask questions.
    return false;
}