#include "config_defs.h"
#include "config.h"
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

#include "eom-hal.h"
#include "esp_err.h"
#include "esp_log.h"
#include "SDHelper.h"
#include "nvs.h"

#include "polyfill.h"

#define MAX_FILE_PATH_LEN 120

#define NVS_NAMESPACE "config"
#define NVS_LAST_CONFIG_FILE "_filename"

static const char* TAG = "config_defs";

extern CONFIG_DEFS;

static esp_err_t _store_last_filename(void) {
    esp_err_t err;
    nvs_handle_t nvs;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) goto cleanup;

    err = nvs_set_str(nvs, NVS_LAST_CONFIG_FILE, Config._filename);
    if (err != ESP_OK) goto cleanup;

    err = nvs_commit(nvs);
    if (err != ESP_OK) goto cleanup;

    ESP_LOGI(TAG, "Set last opened filename to: %s\n", Config._filename);
    nvs_close(nvs);
    return ESP_OK;

cleanup:
    ESP_LOGW(TAG, "Trouble storing current config filename: %s\n", esp_err_to_name(err));
    if (nvs) nvs_close(nvs);
    return err;
}

esp_err_t config_init(void) {
    esp_err_t err;
    nvs_handle_t nvs;

    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (err != ESP_OK) goto defaults;

    size_t len = CONFIG_PATH_MAX;
    err = nvs_get_str(nvs, NVS_LAST_CONFIG_FILE, Config._filename, &len);
    if (err != ESP_OK) goto defaults;

    err = config_load_from_sd(Config._filename, &Config);
    if (err != ESP_OK) goto defaults;

    nvs_close(nvs);
    return ESP_OK;

defaults:
    if (nvs) nvs_close(nvs);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        char path[CONFIG_PATH_MAX + 1] = { 0 };
        SDHelper_getAbsolutePath(path, CONFIG_PATH_MAX, CONFIG_FILENAME);
        err = config_load_from_sd(path, &Config);
    }

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Trouble loading previous config file, falling back to default: %s\n", esp_err_to_name(err));
        config_load_default(&Config);
    }
    
    return err;
}

void config_serialize(config_t* cfg, char* buf, size_t buflen) {
    cJSON* root = cJSON_CreateObject();
    config_to_json(root, cfg);
    cJSON_PrintPreallocated(root, buf, buflen, true);
    cJSON_Delete(root);
}

void config_deserialize(config_t* cfg, const char* buf) {
    cJSON* root = cJSON_Parse(buf);
    json_to_config(root, cfg);
    cJSON_Delete(root);
}

void config_to_json(cJSON* root, config_t* cfg) {
    _config_defs(false, root, cfg, NULL, NULL, NULL, NULL, NULL);
}

void json_to_config(cJSON* root, config_t* cfg) {
    _config_defs(true, root, cfg, NULL, NULL, NULL, NULL, NULL);

    // Bluetooth and WiFi cannot operate at the same time.
    if (cfg->wifi_on && cfg->bt_on && !cfg->force_bt_coex) {
        ESP_LOGW(TAG, "Not enough memory for WiFi and Bluetooth, disabling BT.");
        cfg->bt_on = false;
    }
}

bool set_config_value(const char* option, const char* value, bool* require_reboot) {
    if (_config_defs(false, NULL, &Config, option, value, NULL, NULL, require_reboot)) {
        return true;
    }

    // if (!strcmp(option, "wifi_on")) {
    //   Config.wifi_on = atob(value);
    //   require_reboot = true;
    // } else if(!strcmp(option, "bt_on")) {
    //   Config.bt_on = atob(value);
    //   require_reboot = true;
    // } else if(!strcmp(option, "force_bt_coex")) {
    //   Config.force_bt_coex = atob(value);
    // } else if(!strcmp(option, "led_brightness")) {
    //   Config.led_brightness = atoi(value);
    // } else if(!strcmp(option, "websocket_port")) {
    //   Config.websocket_port = atoi(value);
    //   require_reboot = true;
    // } else if(!strcmp(option, "motor_max_speed")) {
    //   Config.motor_max_speed = atoi(value);
    // } else if(!strcmp(option, "motor_start_speed")) {
    //   Config.motor_start_speed = atoi(value);
    // } else if(!strcmp(option, "edge_delay")) {
    //   Config.edge_delay = atoi(value);
    // } else if(!strcmp(option, "max_additional_delay")) {
    //   Config.max_additional_delay = atoi(value);
    // } else if(!strcmp(option, "minimum_on_time")) {
    //   Config.minimum_on_time = atoi(value);
    // } else if(!strcmp(option, "screen_dim_seconds")) {
    //   Config.screen_dim_seconds = atoi(value);
    // } else if(!strcmp(option, "screen_timeout_seconds")) {
    //   Config.screen_timeout_seconds = atoi(value);
    // } else if(!strcmp(option, "pressure_smoothing")) {
    //   Config.pressure_smoothing = atoi(value);
    // } else if(!strcmp(option, "classic_serial")) {
    //   Config.classic_serial = atob(value);
    // } else if(!strcmp(option, "use_average_values")) {
    //   Config.use_average_values = atob(value);
    // } else if(!strcmp(option, "sensitivity_threshold")) {
    //   Config.sensitivity_threshold = atoi(value);
    // } else if(!strcmp(option, "motor_ramp_time_s")) {
    //   Config.motor_ramp_time_s = atoi(value);
    // } else if(!strcmp(option, "update_frequency_hz")) {
    //   Config.update_frequency_hz = atoi(value);
    // } else if(!strcmp(option, "sensor_sensitivity")) {
    //   Config.sensor_sensitivity = atoi(value);
    //   Hardware::setPressureSensitivity(Config.sensor_sensitivity);
    // } else if (!strcmp(option, "knob_rgb")) {
    //   uint32_t color = strtoul(value, NULL, 16);
    //   Hardware::setEncoderColor(CRGB(color));
    // } else if (!strcmp(option, "wifi_ssid")) {
    //   strlcpy(Config.wifi_ssid, value, sizeof(Config.wifi_ssid));
    // } else if (!strcmp(option, "wifi_key")) {
    //   strlcpy(Config.wifi_key, value, sizeof(Config.wifi_key));
    // } else if (!strcmp(option, "bt_display_name")) {
    //   strlcpy(Config.bt_display_name, value, sizeof(Config.bt_display_name));
    // } else if (!strcmp(option, "use_ssl")) {
    //   Config.use_ssl = atob(value);
    //   require_reboot = true;
    // } else if (!strcmp(option, "hostname")) {
    //   strlcpy(Config.hostname, value, sizeof(Config.hostname));
    //   require_reboot = true;
    // } else if (!strcmp(option, "vibration_mode")) {
    //   Config.vibration_mode = (VibrationMode) atoi(value);
    // } else if(!strcmp(option, "clench_pressure_sensitivity")) {
    //   Config.clench_pressure_sensitivity = atoi(value);
    // } else if(!strcmp(option, "clench_threshold_2_orgasm")) {
    //   Config.clench_threshold_2_orgasm = atoi(value);
    // } else if(!strcmp(option, "clench_detector_in_edging")) {
    //   Config.clench_detector_in_edging = atob(value);
    // } else if(!strcmp(option, "auto_edging_duration_minutes")) {
    //   Config.auto_edging_duration_minutes = atoi(value);
    // } else if(!strcmp(option, "post_orgasm_duration_seconds")) {
    //   Config.post_orgasm_duration_seconds = atoi(value);
    // } else if(!strcmp(option, "max_clench_duration")) {
    //   Config.max_clench_duration = atoi(value);
    // } else if(!strcmp(option, "post_orgasm_menu_lock")) {
    //   Config.post_orgasm_menu_lock = atob(value);
    // } else if(!strcmp(option, "edge_menu_lock")) {
    //   Config.edge_menu_lock = atob(value);
    // } else {
    //   return false;
    // }

    return false;
}

bool get_config_value(const char* option, char* buffer, size_t len) {
    if (_config_defs(true, NULL, &Config, option, NULL, buffer, len, NULL)) {
        return true;
    }

    // if (!strcmp(option, "wifi_on")) {
    //   out += String(Config.wifi_on) + '\n';
    // } else if(!strcmp(option, "bt_on")) {
    //   out += String(Config.bt_on) + '\n';
    // } else if(!strcmp(option, "force_bt_coex")) {
    //   out += String(Config.force_bt_coex) + '\n';
    // } else if(!strcmp(option, "led_brightness")) {
    //   out += String(Config.led_brightness) + '\n';
    // } else if(!strcmp(option, "websocket_port")) {
    //   out += String(Config.websocket_port) + '\n';
    // } else if(!strcmp(option, "motor_max_speed")) {
    //   out += String(Config.motor_max_speed) + '\n';
    // } else if(!strcmp(option, "motor_start_speed")) {
    //   out += String(Config.motor_start_speed) + '\n';
    // } else if(!strcmp(option, "edge_delay")) {
    //   out += String(Config.edge_delay) + '\n';
    // } else if(!strcmp(option, "max_additional_delay")) {
    //   out += String(Config.max_additional_delay) + '\n';
    // } else if(!strcmp(option, "minimum_on_time")) {
    //   out += String(Config.minimum_on_time) + '\n';
    // } else if(!strcmp(option, "screen_dim_seconds")) {
    //   out += String(Config.screen_dim_seconds) + '\n';
    // } else if(!strcmp(option, "screen_timeout_seconds")) {
    //   out += String(Config.screen_timeout_seconds) + '\n';
    // } else if(!strcmp(option, "pressure_smoothing")) {
    //   out += String(Config.pressure_smoothing) + '\n';
    // } else if(!strcmp(option, "classic_serial")) {
    //   out += String(Config.classic_serial) + '\n';
    // } else if(!strcmp(option, "use_average_values")) {
    //   out += String(Config.use_average_values) + '\n';
    // } else if(!strcmp(option, "sensitivity_threshold")) {
    //   out += String(Config.sensitivity_threshold) + '\n';
    // } else if(!strcmp(option, "motor_ramp_time_s")) {
    //   out += String(Config.motor_ramp_time_s) + '\n';
    // } else if(!strcmp(option, "update_frequency_hz")) {
    //   out += String(Config.update_frequency_hz) + '\n';
    // } else if(!strcmp(option, "sensor_sensitivity")) {
    //   out += String(Config.sensor_sensitivity) + '\n';
    // } else if (!strcmp(option, "knob_rgb")) {
    //   out += ("Usage: set knob_rgb 0xFFCCAA") + '\n';
    // } else if (!strcmp(option, "wifi_ssid")) {
    //   out += String(Config.wifi_ssid) + '\n';
    // } else if (!strcmp(option, "wifi_key")) {
    //   out += String(Config.wifi_key) + '\n';
    // } else if (!strcmp(option, "bt_display_name")) {
    //   out += String(Config.bt_display_name) + '\n';
    // } else if (!strcmp(option, "use_ssl")) {
    //   out += String(Config.use_ssl) + '\n';
    // } else if (!strcmp(option, "hostname")) {
    //   out += String(Config.hostname) + '\n';
    // } else if (!strcmp(option, "vibration_mode")) {
    //   out += String((int) Config.vibration_mode) + '\n';
    // } else if(!strcmp(option, "clench_pressure_sensitivity")) { 
    //   out += String(Config.clench_pressure_sensitivity) + '\n';
    // } else if(!strcmp(option, "clench_threshold_2_orgasm")) {
    //   out += String(Config.clench_threshold_2_orgasm) + '\n';
    // } else if(!strcmp(option, "clench_detector_in_edging")) {
    //   out += String(Config.clench_detector_in_edging) + '\n';
    // } else if(!strcmp(option, "auto_edging_duration_minutes")) {
    //   out += String(Config.auto_edging_duration_minutes) + '\n';
    // } else if(!strcmp(option, "post_orgasm_duration_seconds")) {
    //   out += String(Config.post_orgasm_duration_seconds) + '\n';
    // } else if(!strcmp(option, "max_clench_duration")) {
    //   out += String(Config.max_clench_duration) + '\n';
    // } else if(!strcmp(option, "post_orgasm_menu_lock")) {
    //   out += String(Config.post_orgasm_menu_lock) + '\n';
    // } else if(!strcmp(option, "edge_menu_lock")) {
    //   out += String(Config.edge_menu_lock) + '\n';
    // } else {
    //   return false;
    // }

    return false;
}

void config_enqueue_save(long save_at_ms) {
    static long save_at_ms_tick = 0;

    if (save_at_ms > 0) {
        // Queue a future save:
        save_at_ms_tick = save_at_ms > save_at_ms_tick ? save_at_ms : save_at_ms_tick;
    } else if (save_at_ms < 0) {
        if (save_at_ms_tick > 0 && save_at_ms_tick < millis()) {
            if (Config._filename[0] == '\0') {
                ESP_LOGW(TAG, "Attempt to save config without a _filename!");
                return;
            }

            ESP_LOGI(TAG, "Saving now from future save queue...\n");
            config_save_to_sd(Config._filename, &Config);
            save_at_ms_tick = 0;
        }
    }
}

/**
 * Cast a string to a bool. Accepts "false", "no", "off", "0" to false, all other
 * strings cast to true.
 * @param a
 * @return
 */
bool atob(const char* a) {
    return !(
        strcasecmp(a, "false") == 0 ||
        strcasecmp(a, "no") == 0 ||
        strcasecmp(a, "off") == 0 ||
        strcasecmp(a, "0") == 0
        );
}

// Here There Be Dragons

esp_err_t config_load_from_sd(const char* path, config_t* cfg) {
    FILE* f = fopen(path, "r");

    // Sometimes the first file read fails:
    if (!f) {
        f = fopen(path, "r");
    }

    if (f) {
        long fsize;
        char* buffer;
        size_t result;

        if (f == NULL) {
            ESP_LOGE(TAG, "Failed to open file for reading: %s", path);
            return ESP_FAIL;
        }

        fseek(f, 0, SEEK_END);
        fsize = ftell(f);
        rewind(f);

        ESP_LOGD(TAG, "Allocating %ld bytes for file: %s", fsize, path);
        buffer = (char*) malloc(fsize + 1);

        if (!buffer) {
            fclose(f);
            ESP_LOGE(TAG, "Failed to allocate memory for file: %s", path);
            return ESP_FAIL;
        }

        result = fread(buffer, 1, fsize, f);
        buffer[fsize] = '\0';

        if (result != fsize) {
            free(buffer);
            fclose(f);
            ESP_LOGE(TAG, "Failed to read file: %s", path);
            return ESP_FAIL;
        }

        ESP_LOGD(TAG, "Loaded %ld bytes from %s\n%s", fsize, path, buffer);

        config_deserialize(cfg, buffer);

        // Update config path:
        strlcpy(cfg->_filename, path, CONFIG_PATH_MAX);

        if (&Config == cfg) {
            _store_last_filename();
        }

        fclose(f);
        free(buffer);

        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "File does not exist: %s", path);
        return ESP_ERR_NOT_FOUND;
    }
}

esp_err_t config_save_to_sd(const char* path, config_t* cfg) {
    cJSON* root = cJSON_CreateObject();
    struct stat st;

    config_to_json(root, cfg);

    // Delete old config if it exists:
    if (stat(path, &st) == 0) {
        char bak_path[MAX_FILE_PATH_LEN + 1] = "";
        strlcpy(bak_path, path, MAX_FILE_PATH_LEN);
        strlcat(bak_path, ".bak", MAX_FILE_PATH_LEN);

        unlink(bak_path);

        if (rename(path, bak_path) != 0) {
            ESP_LOGE(TAG, "Failed to rename %s to %s", path, bak_path);
            return ESP_FAIL;
        }
    }

    FILE* f = fopen(path, "w");

    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", path);
        return ESP_FAIL;
    }

    char* json = cJSON_Print(root);
    fputs(json, f);
    long fsize = ftell(f);
    fclose(f);

    ESP_LOGD(TAG, "Wrote %ld bytes:\n%s", fsize, json);

    // Update config path:
    if (strcmp(path, cfg->_filename)) {
        strlcpy(cfg->_filename, path, CONFIG_PATH_MAX);

        // Update our last active config file:
        if (&Config == cfg) {
            _store_last_filename();
        }
    }

    cJSON_Delete(root);
    cJSON_free(json);

    return ESP_OK;
}

void config_load_default(config_t* cfg) {
    cJSON* root = cJSON_CreateObject();
    json_to_config(root, cfg);
    SDHelper_getAbsolutePath(cfg->_filename, CONFIG_PATH_MAX, CONFIG_FILENAME);
    cJSON_Delete(root);
}