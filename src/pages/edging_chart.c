
#include "application.h"
#include "assets.h"
#include "eom-hal.h"
#include "esp_log.h"
#include "orgasm_control.h"
#include "ui/graphics.h"
#include "ui/page.h"
#include "ui/toast.h"
#include "util/i18n.h"
#include "version.h"

#define AXIS_WIDTH 3
#define CHART_WIDTH (EOM_DISPLAY_WIDTH - AXIS_WIDTH)
#define CHART_HEIGHT (EOM_DISPLAY_HEIGHT - (UI_BUTTON_HEIGHT * 2) - 4 - AXIS_WIDTH)

static const char* TAG = "page:splash";

struct chart_data {
    uint8_t speed;
    uint16_t arousal;
};

static struct chart_data* data = NULL;
static size_t idx = 0;

static void on_open(void* arg) {
    data = calloc(CHART_WIDTH, sizeof(struct chart_data));
    if (data == NULL) ui_toast("E_NO_MEM");
}

static ui_render_flag_t on_loop(void* arg) {
    static unsigned long last_update_ms = 0;
    unsigned long millis = esp_timer_get_time() / 1000UL;

    if (data == NULL) return NORENDER;

    if (millis - last_update_ms > 1000UL / 15) {
        last_update_ms = millis;
        data[idx].arousal = orgasm_control_getArousal();
        data[idx].speed = eom_hal_get_motor_speed();
        idx = (idx + 1) % CHART_WIDTH;

        return RENDER;
    }

    return NORENDER;
}

static void on_render(u8g2_t* d, void* arg) {
    ui_draw_status(d, _("Arousal Chart"));
    ui_draw_button_labels(d, _("BACK"), NULL, NULL);
    ui_draw_button_disable(d, 0b011);

    if (data == NULL) return;

    u8g2_SetDrawColor(d, 1);

    // Render Final Chart
    const uint8_t sx = AXIS_WIDTH;
    const uint8_t sy = UI_BUTTON_HEIGHT + 2;
    u8g2_DrawVLine(d, sx - 1, sy, CHART_HEIGHT);
    u8g2_DrawHLine(d, sx - 1, sy + CHART_HEIGHT, CHART_WIDTH);

    for (int i = CHART_HEIGHT; i > 0; i--) {
        if (i % 5 == 0) u8g2_DrawHLine(d, 0, sy + i, AXIS_WIDTH);
    }

    uint8_t last_y = 255;

    for (int i = 0; i < CHART_WIDTH; i++) {
        struct chart_data* datum = &data[(i + idx) % CHART_WIDTH];
        if (i % 5 == 0) u8g2_DrawVLine(d, sx + i, sy + CHART_HEIGHT, AXIS_WIDTH);

        uint16_t arousal_max = orgasm_control_get_arousal_threshold();
        arousal_max = arousal_max + (arousal_max / 2); // down with floats, * 1.5

        uint8_t arousal_y = (datum->arousal * CHART_HEIGHT) / arousal_max;
        if (arousal_y > CHART_HEIGHT) arousal_y = CHART_HEIGHT;

        if (last_y < arousal_y) {
            u8g2_DrawVLine(d, sx + i, sy + (CHART_HEIGHT - arousal_y), arousal_y - last_y);
        } else if (last_y == 255 || last_y == arousal_y) {
            u8g2_DrawVLine(d, sx + i, sy + (CHART_HEIGHT - arousal_y), 1);
        } else {
            u8g2_DrawVLine(d, sx + i, sy + (CHART_HEIGHT - last_y), last_y - arousal_y);
        }

        last_y = arousal_y;
    }
}

static void on_close(void* arg) {
    free(data);
    data = NULL;
}

static ui_render_flag_t
on_button(eom_hal_button_t button, eom_hal_button_event_t event, void* arg) {
    if (event != EOM_HAL_BUTTON_PRESS || button != EOM_HAL_BUTTON_BACK) return PASS;
    ui_open_page(&PAGE_EDGING_STATS, NULL);
    return NORENDER;
}

const struct ui_page EDGING_CHART_PAGE = {
    .title = "Arousal Chart",
    .on_open = on_open,
    .on_close = on_close,
    .on_render = on_render,
    .on_loop = on_loop,
    .on_encoder = NULL,
    .on_button = on_button,
};