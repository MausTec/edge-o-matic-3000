#include "application.h"
#include "miniz.h"
#include "esp_log.h"
#include "cJSON.h"
#include "eom-hal.h"
#include <stdio.h>

static const char *TAG = "application";

app_err_t application_load(const char* path, application_t *app) {
    mz_zip_archive archive;
    mz_bool status = 1;
    app_err_t err = APP_OK;
    cJSON *manifest_json = NULL;
    char *manifest = NULL;
    char tmp_ep_path[PATH_MAX] = "";
    int mb_err = MB_FUNC_OK;

    memset(app, 0, sizeof(application_t));
    memset(&archive, 0, sizeof(archive));

    ESP_LOGI(TAG, "Loading %s/manifest.json from archive...", path);

    status = mz_zip_reader_init_file(&archive, path, 0);
    if (!status) goto cleanup;

    ESP_LOGI(TAG, "Locating manifest...");

    size_t fcount = mz_zip_reader_get_num_files(&archive);
    ESP_LOGI(TAG, "Archive has %d files.", fcount);

    for (size_t i = 0; i < fcount; i++) {
        char fname[100] = "";
        mz_zip_reader_get_filename(&archive, i, fname, 99);
        ESP_LOGI(TAG, "- %s", fname);
    }

    size_t manifest_size = 0;
    manifest = mz_zip_reader_extract_file_to_heap(&archive, "manifest.json", &manifest_size, 0);

    if (manifest == NULL) {
        status = 0;
        goto cleanup;
    }

    ESP_LOGI(TAG, "Manifest: %.*s", manifest_size, (char*)manifest);

    manifest_json = cJSON_ParseWithLength(manifest, manifest_size);

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

        status = mz_zip_reader_extract_file_to_file(&archive, entrypoint->valuestring, tmp_ep_path, 0);
        
        if (!status) {
            err = APP_NO_ENTRYPOINT;
            goto cleanup;
        }
    }

    { // Parse Script
        mb_init();
        mb_open(&app->interpreter);
        mb_err = mb_load_file(app->interpreter, tmp_ep_path);
        
        if (mb_err != MB_FUNC_OK) {
            err = APP_NO_ENTRYPOINT;
            goto cleanup;
        }
    }

cleanup:
    if (!status) {
        mz_zip_error zerr = mz_zip_get_last_error(&archive);
        err = APP_FILE_INVALID;
        ESP_LOGE(TAG, "Zip error %d: %s", zerr, mz_zip_get_error_string(zerr));
    }

    free(manifest);
    mz_zip_reader_end(&archive);
    cJSON_free(manifest_json);
    remove(tmp_ep_path);

    return err;
}

app_err_t application_start(application_t *app) {
    if (app->interpreter == NULL)
        return APP_FILE_NOT_FOUND;

    int mb_err = mb_run(app->interpreter, true);
    if (mb_err != MB_FUNC_OK) return APP_START_NO_MEMORY;

    return APP_OK;
}

app_err_t application_kill(application_t *app) {
    return APP_FILE_NOT_FOUND;
}

void app_dispose(application_t *app);