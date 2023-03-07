#include "util/fs.h"
#include "eom-hal.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "util:fs";

size_t fs_read_dir(
    const char* path, fs_read_dir_result_t result, enum fs_read_dir_flags flags, void* arg
) {
    DIR* d;
    struct dirent* dir;
    d = opendir(path);
    size_t count = 0;

    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (flags & FS_READ_NO_HIDDEN && dir->d_name[0] == '.') continue;
            char* file = NULL;

            if (flags & FS_READ_ABSOLUTE_PATH) {
                size_t sz = strlen(dir->d_name) + strlen(path) + 2;
                file = (char*)malloc(sz);
                if (file == NULL) break;
                sprintf(file, "%s/%s", path, dir->d_name);
            } else {
                size_t sz = strlen(dir->d_name) + 1;
                file = (char*)malloc(sz);
                if (file == NULL) break;
                strcpy(file, dir->d_name);
            }

            if (result != NULL) result(file, dir, arg);
            if (~flags & FS_READ_NO_FREE) free(file);
            count++;
        }
    }

    closedir(d);
    return count;
}

char* fs_read_file(const char* path) {
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

const char* fs_sd_root(void) {
    return eom_hal_get_sd_mount_point();
}

int fs_strcmp_ext(const char* path, const char* extension) {
    if (extension[0] == '.') extension++;
    const char* path_ext = path;

    while (*path != '\0') {
        if (*path == '.') path_ext = path + 1;
        path++;
    }

    ESP_LOGD(TAG, "fs_strcmp_ext(\"%s\", \"%s\")", path_ext, extension);
    return strcmp(path_ext, extension);
}

size_t fs_write_file(const char* path, const char* data) {
    if (path == NULL || data == NULL) return 0;

    ESP_LOGI(TAG, "fs_write_file(\"%s\", \"%s\")", path, data);
    FILE* fp;
    size_t len = 0;

    fp = fopen(path, "w+");
    if (fp != NULL) {
        len = fputs(data, fp);
        fclose(fp);
    }

    return len;
}