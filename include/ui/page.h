#ifndef __ui__page_h
#define __ui__page_h

#ifdef __cplusplus
extern "C" {
#endif

#include "eom-hal.h"

#define UI_PAGE_TITLE_MAX 64

typedef struct ui_page ui_page_t;

typedef void (*ui_page_event_cb)(struct ui_page *p);
typedef void (*ui_page_render_cb)(u8g2_t *display, struct ui_page *p);
typedef void (*ui_page_encoder_cb)(int difference, struct ui_page *p);
typedef void (*ui_page_button_cb)(eom_hal_button_t button, eom_hal_button_event_t evt, struct ui_page *p);

struct ui_page {
    char title[UI_PAGE_TITLE_MAX];
    ui_page_event_cb on_open;
    ui_page_render_cb on_render;
    ui_page_event_cb on_loop;
    ui_page_event_cb on_close;
    ui_page_button_cb on_button;
    ui_page_encoder_cb on_encoder;
};

/**
 * Register your pages here.
 */

extern const ui_page_t pSNAKE;

#ifdef __cplusplus
}
#endif

#endif
