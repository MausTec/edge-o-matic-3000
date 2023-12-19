#include "ui/screenshot.h"
#include "eom-hal.h"
#include "esp_log.h"
#include "tlibmp.h"
#include <errno.h>
#include <string.h>
#include <time.h>

static const char* TAG = "ui:screenshot";
static FILE* screenshot_fh = NULL;

static void _display_iterate(
    u8g2_t* display, void (*out)(uint16_t x, uint16_t y, uint8_t color, void* arg), void* arg
) {
    uint8_t* buffer = u8g2_GetBufferPtr(display);
    uint8_t tw = u8g2_GetBufferTileWidth(display);
    uint8_t th = u8g2_GetBufferTileHeight(display);
    uint16_t x, y;
    uint16_t w, h;
    uint8_t c;

    w = tw * 8;
    h = th * 8;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            c = u8x8_capture_get_pixel_1(w - x - 1, h - y - 1, buffer, tw);
            out(x, y, c, arg);
        }
    }
}

void _bmp_iterate_cb(uint16_t x, uint16_t y, uint8_t color, void* arg) {
    tlb_pixel_set((tlb_image_t*)arg, x, y, color ? tlb_rgb(255, 255, 255) : tlb_rgb(0, 0, 0));
}

size_t ui_screenshot_save_bmp(const char* filename) {
    tlb_image_t* image = NULL;
    size_t sz = 0;
    const int width = eom_hal_get_display_width();
    const int height = eom_hal_get_display_height();
    u8g2_t* display = eom_hal_get_display_ptr();

    image = tlb_img_new(width, height, tlb_rgb(0, 0, 0));
    if (image == NULL) {
        ESP_LOGW(TAG, "Failed to initialize BMP image.");
        goto cleanup;
    }

    uint8_t tw = u8g2_GetBufferTileWidth(display);
    uint8_t th = u8g2_GetBufferTileHeight(display);

    _display_iterate(display, _bmp_iterate_cb, image);
    tlb_save_bmp(image, filename);

cleanup:
    if (image) tlb_img_free(image);
    return sz;
}

static void _write_screenshot(const char* out) {
    fputs(out, screenshot_fh);
}

size_t ui_screenshot_save(const char* filename) {
    u8g2_t* display = eom_hal_get_display_ptr();
    char* tmpfile_name;
    size_t len = 0;

    if (eom_hal_get_sd_size_bytes() <= 0) {
        ESP_LOGW(TAG, "No SD card inserted, this does nothing!");
        return 0;
    }

    if (filename == NULL) {
        time_t t = time(NULL);
        struct tm* tm = localtime(&t);

        asiprintf(
            &tmpfile_name,
            "%s/screenshot-%04d%02d%02d-%02d%02d%02d.bmp",
            eom_hal_get_sd_mount_point(),
            tm->tm_year + 1900,
            tm->tm_mon + 1,
            tm->tm_mday,
            tm->tm_hour,
            tm->tm_min,
            tm->tm_sec
        );

        if (tmpfile_name == NULL) goto cleanup;
        ESP_LOGI(TAG, "Writing screenshot %s...", tmpfile_name);
        ui_screenshot_save_bmp(tmpfile_name);
    } else {
        ESP_LOGI(TAG, "Writing screenshot %s...", filename);
        ui_screenshot_save_bmp(filename);
    }

cleanup:
    if (tmpfile_name) free(tmpfile_name);
    return len;
}