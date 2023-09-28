#include "config_defs.h"
#include "config.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "SDHelper.h"
#include "eom-hal.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"

#include "api/broadcast.h"

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
        ESP_LOGW(
            TAG,
            "Trouble loading previous config file, falling back to default: %s\n",
            esp_err_to_name(err)
        );
        config_load_default(&Config);
    }

    return err;
}

// get_config_to_string
void config_serialize(config_t* cfg, char* buf, size_t buflen) {
    cJSON* root = cJSON_CreateObject();
    config_to_json(root, cfg);
    cJSON_PrintPreallocated(root, buf, buflen, true);
    cJSON_Delete(root);
}

// set_config_from_string
void config_deserialize(config_t* cfg, const char* buf) {
    cJSON* root = cJSON_Parse(buf);
    json_to_config(root, cfg);
    cJSON_Delete(root);
}

// get_config_to_json
void config_to_json(cJSON* root, config_t* cfg) {
    _config_defs(CFG_GET, root, cfg, NULL, NULL, NULL, 0, NULL);
}

static void _validate_config(config_t* cfg) {
    // Bluetooth and WiFi cannot operate at the same time.
    if (cfg->wifi_on && cfg->bt_on && !cfg->force_bt_coex) {
        ESP_LOGW(TAG, "Not enough memory for WiFi and Bluetooth, disabling BT.");
        cfg->bt_on = false;
    }

    // Update hardware:
    eom_hal_set_sensor_sensitivity(cfg->sensor_sensitivity);
}

// set_config_from_json
void json_to_config(cJSON* root, config_t* cfg) {
    _config_defs(CFG_SET, root, cfg, NULL, NULL, NULL, 0, NULL);
    _validate_config(cfg);
}

// merge_config_from_json
void json_to_config_merge(cJSON* root, config_t* cfg) {
    _config_defs(CFG_MERGE, root, cfg, NULL, NULL, NULL, 0, NULL);
    _validate_config(cfg);
}

bool set_config_value(const char* option, const char* value, bool* require_reboot) {
    if (_config_defs(CFG_SET, NULL, &Config, option, value, NULL, 0, require_reboot)) {
        _validate_config(&Config);
        return true;
    }

    return false;
}

bool get_config_value(const char* option, char* buffer, size_t len) {
    if (_config_defs(CFG_GET, NULL, &Config, option, NULL, buffer, len, NULL)) {
        return true;
    }

    return false;
}

/**
 * @brief Enqueue a save after a particular delay. To check or tick, pass -1 for arg.
 *
 * @param delay Delay in MS after which to save, to prevent repeated duplicates. 0 to save now, -1
 * to tick.
 */
void config_enqueue_save(long delay) {
    static unsigned long save_at_ms_tick = 0;
    unsigned long millis = esp_timer_get_time() / 1000UL;

    if (delay > 0) {
        // Queue a future save:
        save_at_ms_tick = millis + delay;
    } else {
        if (delay == 0 || (save_at_ms_tick > 0 && save_at_ms_tick < millis)) {
            if (Config._filename[0] == '\0') {
                ESP_LOGW(TAG, "Attempt to save config without a _filename!");
                return;
            }

            ESP_LOGI(
                TAG,
                "Saving now from future save queue... (delay=%ld, at=%ld, ms=%ld)",
                delay,
                save_at_ms_tick,
                millis
            );

            config_save_to_sd(Config._filename, &Config);
            api_broadcast_config();
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
        strcasecmp(a, "false") == 0 || strcasecmp(a, "no") == 0 || strcasecmp(a, "off") == 0 ||
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

    if (!f) {
        ESP_LOGW(TAG, "File does not exist: %s", path);
        return ESP_ERR_NOT_FOUND;
    }

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
    buffer = (char*)malloc(fsize + 1);

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
    ESP_LOGI(TAG, "Loading default config...");
    cJSON* root = cJSON_CreateObject();
    json_to_config(root, cfg);
    SDHelper_getAbsolutePath(cfg->_filename, CONFIG_PATH_MAX, CONFIG_FILENAME);
    cJSON_Delete(root);
}