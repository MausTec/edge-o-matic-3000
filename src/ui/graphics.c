#include "ui/graphics.h"
#include "eom-hal.h"

#include "assets/icons.h"
#include "ui/ui.h"

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

void ui_draw_str_center(u8g2_t* d, uint8_t cx, uint8_t y, const char* str) {
    u8g2_uint_t width = u8g2_GetStrWidth(d, str);
    u8g2_DrawStr(d, cx - (width / 2), y, str);
}

void ui_draw_button_labels(
    u8g2_t* d, const char* left_str, const char* mid_str, const char* right_str
) {
    u8g2_SetDrawColor(d, 1);
    u8g2_DrawBox(
        d,
        (EOM_DISPLAY_WIDTH / 3) * 0,
        EOM_DISPLAY_HEIGHT - UI_BUTTON_HEIGHT,
        EOM_DISPLAY_WIDTH / 3,
        UI_BUTTON_HEIGHT
    );
    u8g2_DrawBox(
        d,
        (EOM_DISPLAY_WIDTH / 3) * 1,
        EOM_DISPLAY_HEIGHT - UI_BUTTON_HEIGHT,
        EOM_DISPLAY_WIDTH / 3,
        UI_BUTTON_HEIGHT
    );
    u8g2_DrawBox(
        d,
        (EOM_DISPLAY_WIDTH / 3) * 2,
        EOM_DISPLAY_HEIGHT - UI_BUTTON_HEIGHT,
        EOM_DISPLAY_WIDTH / 3,
        UI_BUTTON_HEIGHT
    );

    u8g2_SetDrawColor(d, 0);
    u8g2_SetFontPosBaseline(d);
    u8g2_SetFont(d, UI_FONT_SMALL);

    ui_draw_str_center(d, (EOM_DISPLAY_WIDTH / 6) * 1, EOM_DISPLAY_HEIGHT - 1, left_str);
    ui_draw_str_center(d, (EOM_DISPLAY_WIDTH / 6) * 3, EOM_DISPLAY_HEIGHT - 1, mid_str);
    ui_draw_str_center(d, (EOM_DISPLAY_WIDTH / 6) * 5, EOM_DISPLAY_HEIGHT - 1, right_str);
}

void ui_draw_button_disable(u8g2_t* d, uint8_t btnmsk) {
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
    if (icon >= _UI_ICON_MAX)
        return;
    if (icon_states[icon].index != state) {
        icon_states[icon].index = state;
        ui_redraw_all();
    }
}

void ui_draw_status(u8g2_t* d, const char* status) {
    u8g2_SetDrawColor(d, 1);
    u8g2_SetFontPosTop(d);
    u8g2_SetFont(d, UI_FONT_SMALL);
    u8g2_DrawStr(d, 0, 0, status);
    u8g2_DrawHLine(d, 0, 9, EOM_DISPLAY_WIDTH);
    ui_draw_icons(d);
}