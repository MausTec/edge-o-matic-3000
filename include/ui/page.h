#ifndef __ui__page_h
#define __ui__page_h

#ifdef __cplusplus
extern "C" {
#endif

#include "eom-hal.h"
#include "ui/render.h"

#define UI_PAGE_TITLE_MAX 64

typedef struct ui_page ui_page_t;

typedef ui_render_flag_t (*ui_page_event_cb)(void *arg);
typedef void (*ui_page_render_cb)(u8g2_t *display, void *arg);
typedef void (*ui_page_lifecycle_cb)(void *arg);
typedef ui_render_flag_t (*ui_page_encoder_cb)(int difference, void *arg);
typedef ui_render_flag_t (*ui_page_button_cb)(eom_hal_button_t button, eom_hal_button_event_t evt, void *arg);

struct ui_page {
    char title[UI_PAGE_TITLE_MAX];
    ui_page_lifecycle_cb on_open;
    ui_page_render_cb on_render;
    ui_page_event_cb on_loop;
    ui_page_lifecycle_cb on_close;
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
