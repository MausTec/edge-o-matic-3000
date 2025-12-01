#include "application.h"
#include "cJSON.h"
#include "eom-hal.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <string.h>

static const char* TAG = "application";

static struct app_task_node {
    application_t* app;
    struct app_task_node* next;
}* tasklist = NULL;

/**
 * @brief Parse manifest.json from .mpk directory
 */
static app_err_t application_parse_manifest(const char* pack_path, application_t* app) {
    char path[PATH_MAX] = "";
    long fsize;
    char* buffer;
    size_t result;

    snprintf(path, PATH_MAX, "%s/manifest.json", pack_path);
    strncpy(app->pack_path, pack_path, 255);

    FILE* f = fopen(path, "r");
    if (!f) {
        f = fopen(path, "r"); // Retry once
    }

    if (!f) {
        ESP_LOGW(TAG, "Manifest not found: %s", path);
        return APP_FILE_NOT_FOUND;
    }

    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    rewind(f);

    buffer = (char*)malloc(fsize + 1);

    if (!buffer) {
        fclose(f);
        ESP_LOGE(TAG, "Failed to allocate memory for manifest");
        return APP_FILE_INVALID;
    }

    result = fread(buffer, 1, fsize, f);
    buffer[fsize] = '\0';
    fclose(f);

    if (result != fsize) {
        free(buffer);
        ESP_LOGE(TAG, "Failed to read manifest");
        return APP_FILE_INVALID;
    }

    cJSON* manifest_json = cJSON_ParseWithLength(buffer, fsize);
    free(buffer);

    if (manifest_json == NULL) {
        return APP_FILE_INVALID;
    }

    // Get title
    cJSON* title = cJSON_GetObjectItem(manifest_json, "title");

    if (title == NULL || !cJSON_IsString(title)) {
        cJSON_Delete(manifest_json);
        return APP_FILE_INVALID;
    }

    strlcpy(app->title, title->valuestring, APP_TITLE_MAXLEN);

    // Get entrypoint
    cJSON* entrypoint = cJSON_GetObjectItem(manifest_json, "entrypoint");

    if (entrypoint == NULL || !cJSON_IsString(entrypoint)) {
        strlcpy(app->entrypoint, "main.wasm", APP_TITLE_MAXLEN); // Default to main.wasm
    } else {
        strlcpy(app->entrypoint, entrypoint->valuestring, APP_TITLE_MAXLEN);
    }

    cJSON_Delete(manifest_json);
    return APP_OK;
}

app_err_t application_load(const char* path, application_t** app_h) {
    ESP_LOGI(TAG, "STUB: Loading application %s (WASM support not yet implemented)", path);

    *app_h = (application_t*)malloc(sizeof(application_t));

    if (*app_h == NULL) {
        ESP_LOGE(TAG, "No memory to load app");
        return APP_START_NO_MEMORY;
    }

    memset(*app_h, 0, sizeof(application_t));
    application_t* app = *app_h;
    app->status = APP_NOT_LOADED;

    app_err_t err = application_parse_manifest(path, app);

    if (err != APP_OK) {
        free(*app_h);
        *app_h = NULL;
        return err;
    }

    ESP_LOGI(TAG, "Parsed manifest: title='%s', entrypoint='%s'", app->title, app->entrypoint);
    ESP_LOGW(TAG, "WASM loading not implemented - application will not run");

    // Register to task list
    struct app_task_node* node = (struct app_task_node*)malloc(sizeof(struct app_task_node));
    
    if (node == NULL) {
        free(*app_h);
        *app_h = NULL;
        return APP_START_NO_MEMORY;
    }

    node->app = app;
    node->next = tasklist;
    tasklist = node;

    return APP_OK;
}

app_err_t application_start(application_t* app) {
    ESP_LOGW(TAG, "STUB: application_start() - not implemented");
    return APP_NO_INTERPRETER;
}

app_err_t application_start_background(application_t* app) {
    ESP_LOGW(TAG, "STUB: application_start_background() - not implemented");
    return APP_NO_INTERPRETER;
}

app_err_t application_kill(application_t* app) {
    if (app == NULL) return APP_FILE_NOT_FOUND;

    ESP_LOGI(TAG, "STUB: Killing application %s", app->title);
    // TODO: Cleanup WASM instance

    return APP_OK;
}

void app_dispose(application_t* app) {
    if (app == NULL) return;

    ESP_LOGI(TAG, "STUB: Disposing application %s", app->title);

    // Remove from tasklist
    struct app_task_node** ptr = &tasklist;
    while (*ptr != NULL) {
        if ((*ptr)->app == app) {
            struct app_task_node* to_free = *ptr;
            *ptr = (*ptr)->next;
            free(to_free);
            break;
        }
        ptr = &(*ptr)->next;
    }

    // TODO: Cleanup WASM resources
    free(app);
}
