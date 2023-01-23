#ifndef __ui__graphics_h
#define __ui__graphics_h

#ifdef __cplusplus
extern "C" {
#endif

#include "u8g2.h"

#define UI_BUTTON_HEIGHT 9
#define UI_FONT_DEFAULT  u8g2_font_mozart_nbp_tf
#define UI_FONT_LARGE    u8g2_font_profont22_tf
#define UI_FONT_SMALL    u8g2_font_squeezed_r7_tr

void ui_draw_str_center(u8g2_t *d, uint8_t cx, uint8_t y, const char *str);
void ui_draw_button_labels(u8g2_t *d, const char *left_str, const char *mid_str, const char *right_str);
void ui_draw_button_disable(u8g2_t *d, uint8_t button);

#ifdef __cplusplus
}
#endif

#endif
