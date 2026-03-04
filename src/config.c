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
    CFG_STRING_PTR(ssl_cacert_path, NULL);
    CFG_STRING_PTR(ssl_prvtkey_path, NULL);
    CFG_STRING(hostname, "eom3k");
    CFG_BOOL(mdns_enabled, true);

    // UI Settings
    CFG_NUMBER(led_brightness, 128);
    CFG_NUMBER(screen_dim_seconds, 0);
    CFG_NUMBER(screen_timeout_seconds, 0);
    CFG_BOOL(enable_screensaver, false);
    CFG_STRING(language_file_name, "");
    CFG_BOOL(reverse_menu_scroll, false);

    // Orgasm Settings
    CFG_NUMBER(motor_max_speed, 128);
    CFG_NUMBER(motor_min_speed, 10);
    CFG_NUMBER(cooldown_delay_ms, 1000);
    CFG_NUMBER(cooldown_random_ms, 1000);
    CFG_NUMBER(arousal_holdoff_ms, 1000);
    CFG_NUMBER(pressure_smoothing, 5);
    CFG_NUMBER(sensitivity_threshold, 600);
    CFG_NUMBER(motor_ramp_time_s, 30);
    CFG_NUMBER(update_frequency_hz, 50);
    CFG_NUMBER(sensor_sensitivity, 128);
    CFG_BOOL(use_average_values, false);
    CFG_NUMBER(arousal_decay_rate, 60);

    // Orgasm Detection Settings
    CFG_ENUM(od_mode, orgasm_detect_mode_t, OD_MODE_AUTO);
    CFG_NUMBER(od_sustained_threshold, 200);
    CFG_NUMBER(od_sustained_fallback_ms, 5000);
    CFG_NUMBER(od_sustained_dropout_ms, 500);
    CFG_NUMBER(od_peak_min_amplitude, 40);
    CFG_NUMBER(od_rhythmic_min_peaks, 4);
    CFG_NUMBER(od_rhythmic_interval_min_ms, 500);
    CFG_NUMBER(od_rhythmic_interval_max_ms, 1200);
    CFG_NUMBER(od_rhythmic_interval_variance_ms, 200);
    CFG_NUMBER(od_rhythmic_timeout_ms, 1500);
    CFG_NUMBER(od_arousal_gate_percent, 70);
    CFG_NUMBER(od_recovery_ms, 3000);
    CFG_BOOL(od_clench_arousal_boost, false);
    CFG_NUMBER(od_clench_arousal_boost_amount, 5);
    CFG_BOOL(od_detection_armed, true);

    // Vibration Settings
    CFG_ENUM(vibration_mode, vibration_mode_t, RampStop);

    // UI Settings — Edging Stats
    CFG_ENUM(denial_count_mode, denial_count_mode_t, DENIAL_COUNT_DECIMAL);

    // Internal system things, only edit if you know what you're doing.
    CFG_STRING_PTR(remote_update_url, REMOTE_UPDATE_URL)

    // Please just leave this here and don't ask questions.
    return false;
}