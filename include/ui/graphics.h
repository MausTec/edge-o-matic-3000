#ifndef __ui__graphics_h
#define __ui__graphics_h

#ifdef __cplusplus
extern "C" {
#endif

#include "assets/icons.h"
#include "eom-hal.h"
#include "u8g2.h"

#define UI_BUTTON_STR_MAX 12
#define UI_BUTTON_HEIGHT 8
#define UI_FONT_DEFAULT u8g2_font_mozart_nbp_tf
#define UI_FONT_LARGE u8g2_font_profont22_tf
#define UI_FONT_SMALL u8g2_font_5x7_tf
#define UI_FONT_SMALL_CYR u8g2_font_5x7_t_cyrillic
#define UI_FONT_JAPANESE u8g2_font_b10_t_japanese1

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
void ui_draw_bar_graph(u8g2_t* d, uint8_t y, const char label, float value, float max);

void ui_draw_shaded_bar_graph(
    u8g2_t* d, uint8_t y, const char label, float value, float max, float shade_max
);

void ui_draw_shaded_bar_graph_with_peak(
    u8g2_t* d, uint8_t y, const char label, float value, float max, float shade_max, float peak_val
);

void ui_draw_shaded_rect(u8g2_t* d, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);

void ui_draw_scrollbar(u8g2_t* d, size_t index, size_t count, size_t window_size);

#ifdef __cplusplus
}
#endif

#endif
