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

static void app_load_task(void *arg) {
    const char *path = ((struct app_load_args*) arg)->path;
    application_t *app = ((struct app_load_args*) arg)->app;

    mz_zip_archive *archive = (mz_zip_archive*) malloc(sizeof(mz_zip_archive));
    mz_bool status = 1;
    app_err_t err = APP_OK;
    cJSON *manifest_json = NULL;
    char *manifest = NULL;
    char tmp_ep_path[PATH_MAX] = "";
    int mb_err = MB_FUNC_OK;

    memset(app, 0, sizeof(application_t));
    memset(archive, 0, sizeof(*archive));

    app->status = APP_NOT_LOADED;

    ESP_LOGI(TAG, "Stack HWMK: %d bytes", uxTaskGetStackHighWaterMark(NULL));
    ESP_LOGI(TAG, "Current: %d bytes", ((void*)&mb_err) - ((void*)pxTaskGetStackStart(NULL)));
    ESP_LOGI(TAG, "Loading %s/manifest.json from archive...", path);

    status = mz_zip_reader_init_file(archive, path, 0);
    if (!status) goto cleanup;

    ESP_LOGI(TAG, "Locating manifest...");

    size_t fcount = mz_zip_reader_get_num_files(archive);
    ESP_LOGI(TAG, "Archive has %d files.", fcount);

    for (size_t i = 0; i < fcount; i++) {
        char fname[100] = "";
        mz_zip_reader_get_filename(archive, i, fname, 99);
        ESP_LOGI(TAG, "- %s", fname);
    }

    size_t manifest_size = 0;
    manifest = mz_zip_reader_extract_file_to_heap(archive, "manifest.json", &manifest_size, 0);

    if (manifest == NULL) {
        status = 0;
        goto cleanup;
    }

    ESP_LOGI(TAG, "Manifest: %.*s", manifest_size, (char*)manifest);

    manifest_json = cJSON_ParseWithLength(manifest, manifest_size);
    free(manifest);

    if (manifest_json == NULL) {
        err = APP_FILE_INVALID;
        goto cleanup;
    }

    { // Get Title
        cJSON *title = cJSON_GetObjectItem(manifest_json, "title");

        if (title == NULL || !cJSON_IsString(title)) {
            err = APP_FILE_INVALID;
            goto cleanup;
        }

        strlcpy(app->title, title->valuestring, APP_TITLE_MAXLEN);
    }

    { // Load Entrypoint
        cJSON *entrypoint = cJSON_GetObjectItem(manifest_json, "entrypoint");

        if (entrypoint == NULL || !cJSON_IsString(entrypoint)) {
            err = APP_NO_ENTRYPOINT;
            goto cleanup;
        }

        size_t len = sniprintf(tmp_ep_path, PATH_MAX - 10, "%s/tmp/app-", eom_hal_get_sd_mount_point());
        for (size_t i = 0; i < 10; i++) tmp_ep_path[i+len] = (i < 9) ? ('a' + rand() % 26) : '\0';
        strlcat(tmp_ep_path, ".tmp", PATH_MAX);

        ESP_LOGI(TAG, "Extracting %s to %s...", entrypoint->valuestring, tmp_ep_path);

        status = mz_zip_reader_extract_file_to_file(archive, entrypoint->valuestring, tmp_ep_path, 0);
        
        if (!status) {
            err = APP_NO_ENTRYPOINT;
            goto cleanup;
        }
    }

    { // Parse Script
        ESP_LOGI(TAG, "Loaded, initializing interpreter...");
        mb_init();
        mb_open(&app->interpreter);
        application_interpreter_hooks(app->interpreter);

        mb_err = mb_load_file(app->interpreter, tmp_ep_path);
        
        if (mb_err != MB_FUNC_OK) {
            err = APP_NO_ENTRYPOINT;
            goto cleanup;
        }
    }

    ESP_LOGI(TAG, "Application loaded.");

cleanup:
    if (!status) {
        mz_zip_error zerr = mz_zip_get_last_error(archive);
        err = APP_FILE_INVALID;
        ESP_LOGE(TAG, "Zip error %d: %s", zerr, mz_zip_get_error_string(zerr));
    }

    mz_zip_reader_end(archive);
    cJSON_free(manifest_json);
    remove(tmp_ep_path);
    free(archive);
    ((struct app_load_args*) arg)->err = err;

    xTaskNotifyGive(((struct app_load_args*) arg)->caller);
    vTaskSuspend(NULL);
}

app_err_t application_load(const char* path, application_t **app) {
    TaskHandle_t load_task;

    // Provision an entry in our task manager. THIS LEAKS RIGHT NOW.
    struct app_task_node *node = (struct app_task_node*) malloc(sizeof(struct app_task_node));
    *app = &node->app;

    struct app_load_args args = {
        .app = &node->app,
        .path = path,
        .err = APP_NOT_LOADED,
        .caller = xTaskGetCurrentTaskHandle(),
    };

    BaseType_t err = xTaskCreate(app_load_task, "apploader", 1024 * 15, &args, tskIDLE_PRIORITY + 1, &load_task);
    
    if (err == pdPASS) {
        ESP_LOGI(TAG, "Waiting for loader to finish:");
        TaskStatus_t lt_status;
    
        do {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            vTaskGetInfo(load_task, &lt_status, pdFALSE, eInvalid);
        } while (lt_status.eCurrentState != eSuspended);

        vTaskDelete(load_task);
    }

    if (args.err != APP_OK) {
        ESP_LOGE(TAG, "App load error: %d", args.err);
        free(node);
        *app = NULL;
    } else {
        if (tasklist == NULL) {
            tasklist = node;
        } else {
            struct app_task_node *ptr = tasklist;
            while (ptr != NULL && ptr->next != NULL) ptr = ptr->next;
            if (ptr != NULL) ptr->next = node;
            node->next = NULL;
        }
        ESP_LOGI(TAG, "App registered with index.");
    }

    return args.err;
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