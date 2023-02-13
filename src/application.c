#include "application.h"
#include "basic_api.h"
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

static void app_run_task(void* arg) {
    application_t* app = (application_t*)arg;

    ESP_LOGI(
        TAG,
        "Executing BASIC program %s. Stack high water: %d bytes free. Heap %d bytes free.",
        app->title,
        uxTaskGetStackHighWaterMark(NULL),
        heap_caps_get_free_size(MALLOC_CAP_8BIT)
    );

    app->status = APP_OK;
    int mb_err = mb_run(app->interpreter, true);

    if (mb_err != MB_FUNC_OK) {
        const char* file = NULL;
        int pos = 0, row = 0, col = 0;
        mb_error_e er = mb_get_last_error(app->interpreter, &file, &pos, &row, &col);
        ESP_LOGE(
            TAG,
            "Basic Error: %d, %s in %s:%d:%d (%d)",
            er,
            mb_get_error_desc(er),
            file,
            row,
            col,
            pos
        );
        app->status = APP_START_NO_MEMORY;
    }

    mb_close(&app->interpreter);
    mb_dispose();

    ESP_LOGI(
        TAG,
        "Application %s terminated. Stack high water: %d bytes free. Status: %d",
        app->title,
        uxTaskGetStackHighWaterMark(NULL),
        app->status
    );
    app->task = NULL;
    vTaskDelete(NULL);
}

/**
 * @brief Loads the application manifest and initializes the pack path for the app.
 *
 * @param pack_path
 * @param app
 * @return app_err_t
 */
app_err_t application_parse_manifest(const char* pack_path, application_t* app) {
    char path[PATH_MAX] = "";
    long fsize;
    char* buffer;
    size_t result;

    sniprintf(path, PATH_MAX, "%s/manifest.json", pack_path);
    strncpy(app->pack_path, pack_path, 255);

    FILE* f = fopen(path, "r");

    // Sometimes the first file read fails:
    if (!f) {
        f = fopen(path, "r");
    }

    if (!f) {
        ESP_LOGW(TAG, "File does not exist: %s", path);
        return APP_FILE_NOT_FOUND;
    }

    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    rewind(f);

    ESP_LOGD(TAG, "Allocating %ld bytes for file: %s", fsize, path);
    buffer = (char*)malloc(fsize + 1);

    if (!buffer) {
        fclose(f);
        ESP_LOGE(TAG, "Failed to allocate memory for file: %s", path);
        return APP_FILE_INVALID;
    }

    result = fread(buffer, 1, fsize, f);
    buffer[fsize] = '\0';
    fclose(f);

    if (result != fsize) {
        free(buffer);
        ESP_LOGE(TAG, "Failed to read file: %s", path);
        return APP_FILE_INVALID;
    }

    cJSON* manifest_json = cJSON_ParseWithLength(buffer, fsize);
    free(buffer);

    if (manifest_json == NULL) {
        return APP_FILE_INVALID;
    }

    { // Get Title
        cJSON* title = cJSON_GetObjectItem(manifest_json, "title");

        if (title == NULL || !cJSON_IsString(title)) {
            cJSON_free(manifest_json);
            return APP_FILE_INVALID;
        }

        strlcpy(app->title, title->valuestring, APP_TITLE_MAXLEN);
    }

    { // Load Entrypoint
        cJSON* entrypoint = cJSON_GetObjectItem(manifest_json, "entrypoint");

        if (entrypoint == NULL || !cJSON_IsString(entrypoint)) {
            strlcpy(app->entrypoint, "main.bas", APP_TITLE_MAXLEN);
        } else {
            strlcpy(app->entrypoint, entrypoint->valuestring, APP_TITLE_MAXLEN);
        }
    }

    cJSON_free(manifest_json);
    return APP_OK;
}

app_err_t application_load(const char* path, application_t** app_h) {
    app_err_t err = APP_OK;
    char tmp_ep_path[PATH_MAX] = "";
    int mb_err = MB_FUNC_OK;

    ESP_LOGI(TAG, "Loading application %s...", path);

    *app_h = (application_t*)malloc(sizeof(application_t));
    application_t* app = *app_h;

    if (app == NULL) {
        ESP_LOGE(TAG, "No memory to load app!");
        return APP_START_NO_MEMORY;
    }

    memset(app, 0, sizeof(application_t));
    app->status = APP_NOT_LOADED;

    ESP_LOGI(TAG, "Locating manifest...");
    err = application_parse_manifest(path, app);

    if (err != APP_OK) {
        goto cleanup;
    }

    { // Parse Script
        ESP_LOGI(TAG, "Loaded, initializing interpreter...");
        mb_init();
        mb_open(&app->interpreter);
        basic_api_register_all(app->interpreter);

        sniprintf(tmp_ep_path, PATH_MAX, "%s/%s", app->pack_path, app->entrypoint);
        mb_err = mb_load_file(app->interpreter, tmp_ep_path);

        if (mb_err != MB_FUNC_OK) {
            err = APP_NO_ENTRYPOINT;
            goto cleanup;
        }
    }

    ESP_LOGI(TAG, "Application loaded.");

    // register to task list
    struct app_task_node* node = (struct app_task_node*)malloc(sizeof(struct app_task_node));

    if (node == NULL) {
        err = APP_FILE_INVALID;
        goto cleanup;
    }

    node->app = app;
    node->next = NULL;

    if (tasklist == NULL) {
        tasklist = node;
    } else {
        struct app_task_node* ptr = tasklist;

        while (ptr != NULL && ptr->next != NULL) {
            ESP_LOGI(TAG, "ptr=%p", ptr);
            ESP_LOGI(TAG, "ptr->next=%p", ptr->next);
            ptr = ptr->next;
        }

        if (ptr != NULL) ptr->next = node;
    }

    ESP_LOGI(TAG, "App registered with index.");

cleanup:
    if (err != APP_OK) {
        ESP_LOGW(TAG, "Application load error: %d", err);
        free(node);
        free(*app_h);
        *app_h = NULL;
    }

    return err;
}

app_err_t application_start_background(application_t* app) {
    if (app->interpreter == NULL) return APP_FILE_NOT_FOUND;

    size_t stack_depth = app->stack_depth;
    if (stack_depth < APP_MIN_STACK) stack_depth = APP_MIN_STACK;
    ESP_LOGI(TAG, "Pre-empt Launch: %s with %d bytes stack.", app->title, stack_depth);
    BaseType_t err =
        xTaskCreate(app_run_task, app->title, stack_depth, app, tskIDLE_PRIORITY, &app->task);

    if (err != pdPASS) {
        return APP_START_NO_MEMORY;
    }

    return APP_OK;
}

app_err_t application_start(application_t* app) {
    if (app->interpreter == NULL) return APP_FILE_NOT_FOUND;

    app->status = APP_OK;
    int mb_err = mb_run(app->interpreter, true);

    if (mb_err != MB_FUNC_OK) {
        const char* file = NULL;
        int pos = 0, row = 0, col = 0;
        mb_error_e er = mb_get_last_error(app->interpreter, &file, &pos, &row, &col);
        ESP_LOGE(
            TAG,
            "Basic Error: %d, %s in %s:%d:%d (%d)",
            er,
            mb_get_error_desc(er),
            file,
            row,
            col,
            pos
        );
        app->status = APP_START_NO_MEMORY;
        return APP_FILE_INVALID;
    }

    return APP_OK;
}

app_err_t application_kill(application_t* app) {
    if (app == NULL) return APP_FILE_NOT_FOUND;
    mb_close(app->interpreter);
    mb_dispose();
    return APP_OK;
}

void app_dispose(application_t* app);

static void _mb_handle_error(
    struct mb_interpreter_t* s,
    mb_error_e err,
    const char* file,
    const char* idk,
    int wat,
    unsigned short srsly,
    unsigned short wtf,
    int whomst
) {
    ESP_LOGW(TAG, "Syntax Error in %s: (%d)", file, err);
    ESP_LOGW(TAG, "idk=%s, wat=%d, srsly=%d, wtf=%d, whomst=%d", idk, wat, srsly, wtf, whomst);
}