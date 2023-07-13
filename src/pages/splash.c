#include "application.h"
#include "assets.h"
#include "eom-hal.h"
#include "esp_log.h"
#include "ui/graphics.h"
#include "ui/page.h"
#include "util/i18n.h"
#include "version.h"

static const char* TAG = "page:splash";

static void on_open(void* arg) {
    u8g2_t* d = eom_hal_get_display_ptr();
    u8g2_ClearBuffer(d);
    u8g2_SetDrawColor(d, 1);
    u8g2_DrawBitmap(d, 0, 0, 128 / 8, 64, SPLASH_IMG);
    u8g2_SetFontPosBottom(d);
    u8g2_SetFont(d, UI_FONT_SMALL);
    u8g2_DrawStr(d, 0, EOM_DISPLAY_HEIGHT, EOM_VERSION);
    u8g2_SendBuffer(d);
}

const struct ui_page SPLASH_PAGE = {
    .title = "BOOT",
    .on_open = on_open,
    .on_render = NULL,
    .on_close = NULL,
    .on_loop = NULL,
    .on_encoder = NULL,
    .on_button = NULL,
};