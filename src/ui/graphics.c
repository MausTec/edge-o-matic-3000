#include "ui/graphics.h"
#include "eom-hal.h"

#include "assets/icons.h"
#include "esp_log.h"
#include "ui/ui.h"
#include <math.h>

static const char* TAG = "ui:graphcis";

// todo: perhaps this may get moved to its own file, ui/icons.h
static struct status_icon {
    int8_t index;
    const ui_icon_def_t* icon;
} icon_states[] = { {
                        .icon = &SD_ICON,
                        .index = -1,
                    },
                    {
                        .icon = &WIFI_ICON,
                        .index = -1,
                    },
                    {
                        .icon = &BT_ICON,
                        .index = -1,
                    },
                    {
                        .icon = &UPDATE_ICON,
                        .index = -1,
                    },
                    {
                        .icon = &RECORD_ICON,
                        .index = -1,
                    } };

uint8_t ui_draw_str_center(u8g2_t* d, uint8_t cx, uint8_t y, const char* str) {
    u8g2_uint_t width = u8g2_GetUTF8Width(d, str);
    u8g2_DrawUTF8(d, cx - (width / 2), y, str);
    return width;
}

void ui_draw_shaded_rect(u8g2_t* d, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color) {
    u8g2_SetDrawColor(d, color);

    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            if ((i + j) % 2 == 0) u8g2_DrawPixel(d, x + i, y + j);
        }
    }
}

void ui_draw_scrollbar(u8g2_t* d, size_t index, size_t count, size_t window_size) {
    const int title_bar_height = 10;
    const int button_height = 9;
    const int track_height = EOM_DISPLAY_HEIGHT - title_bar_height - button_height - 1;

    int bar_height = track_height / window_size;
    int offset_top = count > 1 ? (track_height - bar_height) * index / (count - 1) : 0;

    u8g2_SetDrawColor(d, 0);
    u8g2_DrawBox(d, EOM_DISPLAY_WIDTH - 3, title_bar_height + 1, 3, track_height);
    u8g2_SetDrawColor(d, 1);
    u8g2_DrawBox(d, EOM_DISPLAY_WIDTH - 2, title_bar_height + 1 + offset_top, 2, bar_height);
}

void ui_draw_button_labels(
    u8g2_t* d, const char* left_str, const char* mid_str, const char* right_str
) {
    u8g2_SetDrawColor(d, 0);
    u8g2_DrawBox(
        d,
        (EOM_DISPLAY_WIDTH / 3) * 0,
        EOM_DISPLAY_HEIGHT - UI_BUTTON_HEIGHT - 1,
        (EOM_DISPLAY_WIDTH),
        UI_BUTTON_HEIGHT + 1
    );
    u8g2_SetDrawColor(d, 1);
    u8g2_DrawBox(
        d,
        (EOM_DISPLAY_WIDTH / 3) * 0,
        EOM_DISPLAY_HEIGHT - UI_BUTTON_HEIGHT,
        (EOM_DISPLAY_WIDTH / 3),
        UI_BUTTON_HEIGHT
    );
    u8g2_DrawBox(
        d,
        ((EOM_DISPLAY_WIDTH / 3) * 1) + 1,
        EOM_DISPLAY_HEIGHT - UI_BUTTON_HEIGHT,
        (EOM_DISPLAY_WIDTH / 3),
        UI_BUTTON_HEIGHT
    );
    u8g2_DrawBox(
        d,
        ((EOM_DISPLAY_WIDTH / 3) * 2) + 2,
        EOM_DISPLAY_HEIGHT - UI_BUTTON_HEIGHT,
        (EOM_DISPLAY_WIDTH / 3),
        UI_BUTTON_HEIGHT
    );

    u8g2_SetDrawColor(d, 0);
    u8g2_SetFontPosBaseline(d);
    u8g2_SetFont(d, UI_FONT_SMALL);

    ui_draw_str_center(d, ((EOM_DISPLAY_WIDTH / 6) * 1) + 0, EOM_DISPLAY_HEIGHT - 1, left_str);
    ui_draw_str_center(d, ((EOM_DISPLAY_WIDTH / 6) * 3) + 1, EOM_DISPLAY_HEIGHT - 1, mid_str);
    ui_draw_str_center(d, ((EOM_DISPLAY_WIDTH / 6) * 5) + 2, EOM_DISPLAY_HEIGHT - 1, right_str);
}

/**
 * @brief I'm not 100% sure this is how I want to handle this, but here we are.
 *
 * @param d
 * @param btnmsk
 */
void ui_draw_button_disable(u8g2_t* d, uint8_t btnmsk) {
    uint8_t i = 0;
    for (uint8_t i = 0; i < 3; i++) {
        if (!(btnmsk & (4 >> i))) continue;

        ui_draw_shaded_rect(
            d,
            ((EOM_DISPLAY_WIDTH / 3) * i) + i,
            EOM_DISPLAY_HEIGHT - UI_BUTTON_HEIGHT,
            (EOM_DISPLAY_WIDTH / 3),
            UI_BUTTON_HEIGHT,
            0
        );
    }
}

void ui_draw_icon(u8g2_t* d, uint8_t idx, const unsigned char* bmp) {
    u8g2_SetDrawColor(d, 1);
    u8g2_DrawBitmap(d, EOM_DISPLAY_WIDTH - (10 * idx) - 8, 0, 1, 8, bmp);
}

void ui_draw_icons(u8g2_t* d) {
    uint8_t idx = 0;

    for (size_t i = 0; i < _UI_ICON_MAX; i++) {
        struct status_icon* icon = &icon_states[i];
        if (icon->index > -1 && icon->index < icon->icon->icon_state_cnt)
            ui_draw_icon(d, idx++, icon->icon->icon_data[icon->index]);
    }
}

void ui_set_icon(ui_icon_t icon, int8_t state) {
    if (icon >= _UI_ICON_MAX) return;
    if (icon_states[icon].index != state) {
        icon_states[icon].index = state;
        ui_redraw_all();
    }
}

void ui_draw_status(u8g2_t* d, const char* status) {
    u8g2_SetDrawColor(d, 1);
    u8g2_SetFontPosTop(d);
    u8g2_SetFont(d, UI_FONT_SMALL);
    u8g2_DrawUTF8(d, 0, 0, status);
    ui_draw_icons(d);
}

void ui_draw_bar_graph(u8g2_t* d, uint8_t y, const char label, float value, float max) {
    uint8_t x = 0;
    uint8_t width = EOM_DISPLAY_WIDTH;
    uint8_t fill_width = max > 0 ? floor((value / max) * (width - 9)) : width - 9;

    u8g2_SetDrawColor(d, 1);

    // Draw label
    u8g2_SetFontPosTop(d);
    u8g2_SetFont(d, UI_FONT_DEFAULT);
    u8g2_DrawGlyph(d, x, y, label);

    // Draw Rect
    u8g2_DrawFrame(d, x + 7, y, width - 7, 9);

    if (value > 0) {
        u8g2_DrawBox(d, x + 8, y + 1, fill_width, 7);
    }
}

void ui_draw_shaded_bar_graph(
    u8g2_t* d, uint8_t y, const char label, float value, float max, float shade_max
) {
    uint8_t x = 0;
    uint8_t width = EOM_DISPLAY_WIDTH;
    uint8_t fill_width = width - 9;
    uint8_t shade_width = max > 0 ? floor((shade_max / max) * fill_width) : fill_width;

    ui_draw_bar_graph(d, y, label, value, max);

    if (shade_max > 0) {
        ui_draw_shaded_rect(d, x + shade_width + 8, y + 2, fill_width - shade_width - 1, 5, 1);
    }
}

void ui_draw_shaded_bar_graph_with_peak(
    u8g2_t* d, uint8_t y, const char label, float value, float max, float shade_max, float peak_val
) {
    uint8_t x = 0;
    uint8_t width = EOM_DISPLAY_WIDTH;
    uint8_t fill_width = width - 9;
    uint8_t peak_width = max > 0 ? floor((peak_val / max) * fill_width) : fill_width;

    ui_draw_shaded_bar_graph(d, y, label, value, max, shade_max);

    if (peak_val > 0) {
        u8g2_DrawVLine(d, x + peak_width + 8, y + 1, 7);
    }
}