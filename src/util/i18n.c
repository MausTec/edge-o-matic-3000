#include "util/i18n.h"
#include "cJSON.h"
#include "config.h"
#include "esp_err.h"
#include "esp_log.h"
#include <stdio.h>

static const char* TAG = "i18n";

#ifdef I18N_USE_CJSON_DICT
static cJSON* language = NULL;
#else
#include "util/hashmap.h"
static hashmap_t* dict;
#endif

char* file_read(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) {
        ESP_LOGW(TAG, "File does not exist: %s", path);
        return NULL;
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
        ESP_LOGE(TAG, "Failed to allocate memory for file: %s", path);
        return NULL;
    }

    result = fread(buffer, 1, fsize, f);
    ESP_LOGD(TAG, "Read %d bytes from %s", result, path);
    buffer[result] = '\0';
    fclose(f);
    return buffer;
}

size_t file_write(const char* path, const char* data) {
    ESP_LOGD(TAG, "Write %s:\n%s", path, data);
    return 0;
}

esp_err_t i18n_load(const char* filename) {
    char full_path[CONFIG_PATH_MAX];
    snprintf(full_path, CONFIG_PATH_MAX, "/sdcard/%s", filename);

    ESP_LOGI(TAG, "Loading language dictionary: %s", full_path);

    char* buffer = file_read(full_path);
    if (buffer == NULL) return ESP_ERR_NOT_FOUND;
    ESP_LOGD(TAG, "Parsing Language Dict: %s", buffer);

#ifndef I18N_USE_CJSON_DICT
    cJSON*
#endif
        language = cJSON_Parse(buffer);

    free(buffer);

    if (language == NULL) {
        ESP_LOGE(TAG, "Language dictionary invalid: %s", full_path);
        return ESP_FAIL;
    }

    if (!cJSON_IsObject(language)) {
        ESP_LOGE(TAG, "Language dictionary invalid: %s", full_path);
#ifdef I18N_USE_CJSON_DICT
        cJSON_free(language);
        language = cJSON_CreateObject();
#endif
        return ESP_ERR_INVALID_ARG;
    }

#ifndef I18N_USE_CJSON_DICT
    // Load in entries:
    cJSON* key = language->child;
    while (key != NULL) {
        hashmap_insert(dict, key->string, key->valuestring);
        key = key->next;
    }

    cJSON_free(language);
#endif
    return ESP_OK;
}

void i18n_init(void) {
#ifndef I18N_USE_CJSON_DICT
    hashmap_init(&dict, 64);
#endif

    if (Config.language_file_name[0] != '\0') {
        i18n_load(Config.language_file_name);
    }

#ifdef I18N_USE_CJSON_DICT
    else {
        language = cJSON_CreateObject();
    }
#endif
}

void i18n_dump(void) {
    if (Config.language_file_name[0] == '\0') return;
#ifdef I18N_USE_CJSON_DICT
    char* buffer = cJSON_Print(language);
    file_write(Config.language_file_name, buffer);
    cJSON_free(buffer);
#endif
}

void i18n_miss(const char* key) {
#ifdef I18N_USE_CJSON_DICT
    cJSON_AddStringToObject(language, key, "");
    ESP_LOGD(TAG, "I18N MISS: \"%s\"", key);
    i18n_dump();
#else
    hashmap_insert(dict, key, "");
#endif
}

const char* _(const char* str) {
#ifdef I18N_USE_CJSON_DICT
    cJSON* obj = cJSON_GetObjectItem(language, str);
    const char* value = obj == NULL ? NULL : obj->valuestring;
#else
    const char* value = hashmap_find(dict, str);
#endif

    if (value == NULL) {
        i18n_miss(str);
        return str;
    }

    if (value[0] == '\0') {
        return str;
    }

    return value;
}