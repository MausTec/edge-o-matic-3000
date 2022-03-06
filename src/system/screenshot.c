#include "system/screenshot.h"
#include "SDHelper.h"
#include "eom-hal.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char* TAG = "system/screenshot";

static SemaphoreHandle_t sem_screenshot = NULL;
static FILE* fh_screenshot = NULL;

static void write_to_screenshot_file(const char* str) { fputs(str, fh_screenshot); }

bool screenshot_save_to_file(char* outpath, size_t len) {
    if (!sem_screenshot)
        sem_screenshot = xSemaphoreCreateMutex();

    if (xSemaphoreTake(sem_screenshot, portMAX_DELAY)) {
        bool success = false;
        char filename[32] = { 0 };

        snprintf(filename, 31, "screenshot-%lu.xbm", (unsigned long)time(NULL));

        fh_screenshot = SDHelper_open(filename, "w");

        if (fh_screenshot != NULL) {
            if (outpath != NULL && len != 0) {
                SDHelper_getAbsolutePath(outpath, len, filename);
            }
            ESP_LOGI(TAG, "Screenshot saved to: %s", filename);
            u8g2_WriteBufferXBM(eom_hal_get_display_ptr(), &write_to_screenshot_file);
            fclose(fh_screenshot);
            success = true;
        }

        xSemaphoreGive(sem_screenshot);
        return success;
    }

    return false;
}