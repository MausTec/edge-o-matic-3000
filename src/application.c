#include "application.h"
#include "miniz.h"
#include "esp_log.h"
#include "cJSON.h"
#include "eom-hal.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "application";

struct app_load_args {
    const char *path;
    application_t *app;
    app_err_t err;
    TaskHandle_t caller;
};

static struct app_task_node {
    application_t app;
    struct app_task_node *next;
} *tasklist = NULL;

static void app_run_task(void *arg) {
    application_t *app = (application_t*) arg;

    ESP_LOGI(TAG, "Executing BASIC program %s. Stack high water: %d bytes free. Heap %d bytes free.", app->title, uxTaskGetStackHighWaterMark(NULL), heap_caps_get_free_size(MALLOC_CAP_8BIT));

    int mb_err = mb_run(app->interpreter, true);
    if (mb_err != MB_FUNC_OK) {
        app->status = APP_START_NO_MEMORY;
        return;
    }

    mb_close(&app->interpreter);
    mb_dispose();

    ESP_LOGI(TAG, "Application %s terminated. Stack high water: %d bytes free.", app->title, uxTaskGetStackHighWaterMark(NULL));
    app->task = NULL;
    app->status = APP_OK;
    vTaskDelete(NULL);
}

/**
 * @brief Loads the application manifest and initializes the pack path for the app.
 * 
 * @param pack_path 
 * @param app 
 * @return app_err_t 
 */
app_err_t application_parse_manifest(const char* pack_path, application_t *app) {
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

    cJSON *manifest_json = cJSON_ParseWithLength(buffer, fsize);
    free(buffer);

    if (manifest_json == NULL) {
        return APP_FILE_INVALID;
    }

    { // Get Title
        cJSON *title = cJSON_GetObjectItem(manifest_json, "title");

        if (title == NULL || !cJSON_IsString(title)) {
            cJSON_free(manifest_json);
            return APP_FILE_INVALID;
        }

        strlcpy(app->title, title->valuestring, APP_TITLE_MAXLEN);
    }

    { // Load Entrypoint
        cJSON *entrypoint = cJSON_GetObjectItem(manifest_json, "entrypoint");

        if (entrypoint == NULL || !cJSON_IsString(entrypoint)) {
            strlcpy(app->entrypoint, "main.bas", APP_TITLE_MAXLEN);
        } else {
            strlcpy(app->entrypoint, entrypoint->valuestring, APP_TITLE_MAXLEN);
        }
    }

    cJSON_free(manifest_json);
    return APP_OK;
}

app_err_t application_load(const char* path, application_t **app_h) {
    app_err_t err = APP_OK;
    char tmp_ep_path[PATH_MAX] = "";
    int mb_err = MB_FUNC_OK;

    ESP_LOGI(TAG, "Loading application %s...", path);

    *app_h = (application_t*) malloc(sizeof(application_t));
    application_t *app = *app_h;
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
        application_interpreter_hooks(app->interpreter);

        sniprintf(tmp_ep_path, PATH_MAX, "%s/%s", app->pack_path, app->entrypoint);
        mb_err = mb_load_file(app->interpreter, tmp_ep_path);
        
        if (mb_err != MB_FUNC_OK) {
            err = APP_NO_ENTRYPOINT;
            goto cleanup;
        }
    }

    ESP_LOGI(TAG, "Application loaded.");

    // register to task list
    struct app_task_node *node = (struct app_task_node*) malloc(sizeof(struct app_task_node));

    if (node == NULL) {
        err = APP_FILE_INVALID;
        goto cleanup;
    }

    if (tasklist == NULL) {
        tasklist = node;
    } else {
        struct app_task_node *ptr = tasklist;
        while (ptr != NULL && ptr->next != NULL) ptr = ptr->next;
        if (ptr != NULL) ptr->next = node;
        node->next = NULL;
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

app_err_t application_start(application_t *app) {
    if (app->interpreter == NULL)
        return APP_FILE_NOT_FOUND;

    size_t stack_depth = app->stack_depth;
    if (stack_depth < APP_MIN_STACK) stack_depth = APP_MIN_STACK;
    ESP_LOGI(TAG, "Pre-empt Launch: %s with %d bytes stack.", app->title, stack_depth);
    BaseType_t err = xTaskCreate(app_run_task, app->title, stack_depth, app, tskIDLE_PRIORITY, &app->task);

    if (err != pdPASS) {
        return APP_START_NO_MEMORY;
    }

    return APP_OK;
}

app_err_t application_kill(application_t *app) {
    return APP_FILE_NOT_FOUND;
}

void app_dispose(application_t *app);

// Abstract thes out to mb_hooks.h / mb_hooks.c

static int _toast(struct mb_interpreter_t* s, void** l) {
    int result = MB_FUNC_OK;
	mb_value_t arg;
    char *str = NULL;

	mb_assert(s && l);
	mb_check(mb_attempt_open_bracket(s, l));
	mb_check(mb_pop_value(s, l, &arg));
	mb_check(mb_attempt_close_bracket(s, l));

	switch(arg.type) {
	case MB_DT_NIL:
		break;

	case MB_DT_INT:
        ESP_LOGI(TAG, "Need to toast: %d", arg.value.integer);
		break;

    case MB_DT_STRING:
        ESP_LOGI(TAG, "Need to toast: %s", arg.value.string);
        break;

	default:
		break;
	}

    mb_check(mb_push_int(s, l, 0));
	return result;
}

static void _mb_handle_error(struct mb_interpreter_t *s, mb_error_e err, const char *file, const char *idk, int wat, unsigned short srsly, unsigned short wtf, int whomst) {
    ESP_LOGW(TAG, "Syntax Error in %s: (%d)", file, err);
    ESP_LOGW(TAG, "idk=%s, wat=%d, srsly=%d, wtf=%d, whomst=%d", idk, wat, srsly, wtf, whomst);
}

void application_interpreter_hooks(struct mb_interpreter_t *s) {
    mb_set_error_handler(s, _mb_handle_error);

    mb_begin_module(s, "UI");
    mb_register_func(s, "TOAST", _toast);
    mb_end_module(s);
}