#include "util/i18n.h"
#include "cJSON.h"
#include "config.h"
#include "esp_err.h"
#include "esp_log.h"
#include "util/fs.h"
#include <stdio.h>

static const char* TAG = "i18n";

#ifdef I18N_USE_CJSON_DICT
static cJSON* language = NULL;
static cJSON* keys = NULL;
#else
#include "util/hashmap.h"
static hashmap_t* dict;
#endif

esp_err_t i18n_load(const char* filename) {
    char full_path[CONFIG_PATH_MAX];
    snprintf(full_path, CONFIG_PATH_MAX, "/sdcard/%s", filename);

    ESP_LOGI(TAG, "Loading language dictionary: %s", full_path);

    char* buffer;
    fs_read_file(full_path, &buffer);
    if (buffer == NULL) return ESP_ERR_NOT_FOUND;
    ESP_LOGD(TAG, "Parsing Language Dict: %s", buffer);

#ifndef I18N_USE_CJSON_DICT
    cJSON*
#endif
        cJSON* language = cJSON_Parse(buffer);

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
    cJSON* keys = cJSON_GetObjectItem(language, "keys");
    cJSON* key = keys->child;
    while (key != NULL) {
        hashmap_insert(dict, key->string, key->valuestring);
        key = key->next;
    }

    cJSON_Delete(language);
#else
    keys = cJSON_GetObjectItem(language, "keys");
    // Fallback for older, improperly formatted files.
    if (keys == NULL) keys = language;
#endif
    return ESP_OK;
}

// todo: ensure re-entrant
esp_err_t i18n_init(void) {
    esp_err_t err = ESP_OK;

#ifndef I18N_USE_CJSON_DICT
    hashmap_init(&dict, 64);
#endif

    if (Config.language_file_name[0] != '\0') {
        err = i18n_load(Config.language_file_name);
    }

#ifdef I18N_USE_CJSON_DICT
    else {
        language = cJSON_CreateObject();
    }
#endif

    return err;
}

void i18n_deinit(void) {
#ifndef I18N_USE_CJSON_DICT
    hashmap_deinit(&dict);
#else
    cJSON_Delete(language);
#endif
}

void i18n_dump(void) {
    if (Config.language_file_name[0] == '\0') return;
#ifdef I18N_USE_CJSON_DICT
    char* buffer = cJSON_Print(language);
    fs_write_file(Config.language_file_name, buffer);
    cJSON_free(buffer);
#endif
}

void i18n_miss(const char* key) {
#ifdef I18N_USE_CJSON_DICT
    cJSON_AddStringToObject(keys, key, "");

#ifdef I18N_WRITE_BACK_UNKNOWN_KEYS
    if (Config.language_file_name[0] != '\0') {
        ESP_LOGD(TAG, "I18N MISS: \"%s\"", key);
        i18n_dump();
    }
#endif
#else
    hashmap_insert(dict, key, "");
#endif
}

const char* _(const char* str) {
    if (Config.language_file_name[0] == '\0') return str;

#ifdef I18N_USE_CJSON_DICT
    cJSON* obj = cJSON_GetObjectItem(keys, str);
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