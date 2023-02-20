#include "util/i18n.h"
#include "cJSON.h"
#include "config.h"
#include "esp_err.h"
#include "esp_log.h"
#include "util/hashmap.h"
#include <stdio.h>

static const char* TAG = "i18n";

static hashmap_t* dict;

esp_err_t i18n_load(const char* filename) {
    char full_path[CONFIG_PATH_MAX];
    snprintf(full_path, CONFIG_PATH_MAX, "/sdcard/%s", filename);

    FILE* f = fopen(full_path, "r");
    if (!f) {
        ESP_LOGW(TAG, "File does not exist: %s", full_path);
        return ESP_ERR_NOT_FOUND;
    }

    size_t fsize;
    char* buffer;
    size_t result;

    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    rewind(f);

    buffer = (char*)malloc(fsize + 1);
    if (!buffer) {
        fclose(f);
        ESP_LOGE(TAG, "Failed to allocate memory for file: %s", full_path);
        return ESP_ERR_NO_MEM;
    }

    result = fread(buffer, 1, fsize, f);
    buffer[fsize] = '\0';
    fclose(f);

    ESP_LOGI(TAG, "Loading language dictionary: %s", full_path);

    cJSON* language = cJSON_ParseWithLength(buffer, fsize);
    free(buffer);

    if (language == NULL) {
        return ESP_FAIL;
    }

    if (!cJSON_IsObject(language)) {
        ESP_LOGE(TAG, "Language dictionary invalid: %s", full_path);
        return ESP_ERR_INVALID_ARG;
    }

    // Load in entries:
    cJSON* key = language->child;
    while (key != NULL) {
        hashmap_insert(dict, key->string, key->valuestring);
        key = key->next;
    }

    cJSON_free(language);
    return ESP_OK;
}

void i18n_init(void) {
    hashmap_init(&dict, 64);

    if (Config.language_file_name[0] != '\0') {
        i18n_load(Config.language_file_name);
    }
}

void i18n_miss(const char* key) {
    ESP_LOGI(TAG, "MISS: \"%s\"", key);
    hashmap_insert(dict, key, "");
}

const char* _(const char* str) {
    const char* value = hashmap_find(dict, str);

    if (value == NULL) {
        i18n_miss(str);
        return str;
    }

    if (value[0] == '\0') {
        return str;
    }

    return value;
}