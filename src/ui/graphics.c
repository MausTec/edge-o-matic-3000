#include "ui/graphics.h"
#include "eom-hal.h"

void ui_draw_str_center(u8g2_t *d, uint8_t cx, uint8_t y, const char *str) {
    u8g2_uint_t width = u8g2_GetStrWidth(d, str);
    u8g2_DrawStr(d, cx - (width / 2), y, str);
}

void ui_draw_button_labels(u8g2_t *d, const char *left_str, const char *mid_str, const char *right_str) {
    u8g2_SetDrawColor(d, 1);
    u8g2_DrawBox(d, (EOM_DISPLAY_WIDTH / 3) * 0, EOM_DISPLAY_HEIGHT - UI_BUTTON_HEIGHT, EOM_DISPLAY_WIDTH / 3, UI_BUTTON_HEIGHT);
    u8g2_DrawBox(d, (EOM_DISPLAY_WIDTH / 3) * 1, EOM_DISPLAY_HEIGHT - UI_BUTTON_HEIGHT, EOM_DISPLAY_WIDTH / 3, UI_BUTTON_HEIGHT);
    u8g2_DrawBox(d, (EOM_DISPLAY_WIDTH / 3) * 2, EOM_DISPLAY_HEIGHT - UI_BUTTON_HEIGHT, EOM_DISPLAY_WIDTH / 3, UI_BUTTON_HEIGHT);
    
    u8g2_SetDrawColor(d, 0);
    u8g2_SetFont(d, UI_FONT_SMALL);
    ui_draw_str_center(d, (EOM_DISPLAY_WIDTH / 6) * 1, EOM_DISPLAY_HEIGHT - UI_BUTTON_HEIGHT, left_str);
    ui_draw_str_center(d, (EOM_DISPLAY_WIDTH / 6) * 3, EOM_DISPLAY_HEIGHT - UI_BUTTON_HEIGHT, mid_str);
    ui_draw_str_center(d, (EOM_DISPLAY_WIDTH / 6) * 5, EOM_DISPLAY_HEIGHT - UI_BUTTON_HEIGHT, right_str);
}

void ui_draw_button_disable(u8g2_t *d, uint8_t button) {

}