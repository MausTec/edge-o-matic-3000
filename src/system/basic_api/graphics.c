#include "basic_api.h"
#include "eom-hal.h"
#include "esp_log.h"
#include "ui/ui.h"
#include <stddef.h>

static const char* TAG = "basic:graphics";

///
/// Should the UI drawing routines in BASIC actually allow for drawing whatever they want, or should
/// it respect the UI's current page render scheme?
///

static int _draw_pixel(struct mb_interpreter_t* s, void** l) {
    int result = MB_FUNC_OK;

    int pix_x = -1;
    int pix_y = -1;
    int pix_color = 0;

    mb_assert(s && l);
    mb_check(mb_attempt_open_bracket(s, l));
    mb_check(mb_pop_int(s, l, &pix_x));
    mb_check(mb_pop_int(s, l, &pix_y));
    mb_check(mb_pop_int(s, l, &pix_color));
    mb_check(mb_attempt_close_bracket(s, l));

    u8g2_t* display = eom_hal_get_display_ptr();
    u8g2_SetDrawColor(display, pix_color);
    u8g2_DrawPixel(display, pix_x, pix_y);

    return result;
}

static int _display_buffer(struct mb_interpreter_t* s, void** l) {
    int result = MB_FUNC_OK;

    mb_assert(s && l);
    mb_check(mb_attempt_func_begin(s, l));
    mb_check(mb_attempt_func_end(s, l));

    void ui_send_buffer(void);
    return result;
}

void basic_api_register_graphics(struct mb_interpreter_t* bas) {
    mb_begin_module(bas, "GRAPHICS");

    // Register namespace here.
    mb_register_func(bas, "DRAW_PIXEL", _draw_pixel);
    mb_register_func(bas, "DISPLAY_BUFFER", _display_buffer);

    mb_end_module(bas);
}