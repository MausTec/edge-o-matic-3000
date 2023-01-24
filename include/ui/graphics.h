#ifndef __ui__graphics_h
#define __ui__graphics_h

#ifdef __cplusplus
extern "C" {
#endif

#include "assets/icons.h"
#include "eom-hal.h"
#include "u8g2.h"

#define UI_BUTTON_HEIGHT 9
#define UI_FONT_DEFAULT u8g2_font_mozart_nbp_tf
#define UI_FONT_LARGE u8g2_font_profont22_tf
#define UI_FONT_SMALL u8g2_font_squeezed_r7_tr

typedef enum ui_icon {
    UI_ICON_SD,
    UI_ICON_WIFI,
    UI_ICON_BT,
    UI_ICON_UPDATE,
    UI_ICON_RECORD,
    _UI_ICON_MAX,
} ui_icon_t;

void ui_draw_str_center(u8g2_t* d, uint8_t cx, uint8_t y, const char* str);
void ui_draw_button_labels(
    u8g2_t* d, const char* left_str, const char* mid_str, const char* right_str
);
void ui_draw_button_disable(u8g2_t* d, uint8_t btnmsk);
void ui_draw_status(u8g2_t* d, const char* status);
void ui_set_icon(ui_icon_t icon, int8_t state);

#ifdef __cplusplus
}
#endif

#endif
