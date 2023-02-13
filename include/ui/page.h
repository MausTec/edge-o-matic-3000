#ifndef __ui__page_h
#define __ui__page_h

#ifdef __cplusplus
extern "C" {
#endif

#include "eom-hal.h"
#include "ui/render.h"

#define UI_PAGE_TITLE_MAX 64

typedef struct ui_page ui_page_t;

typedef ui_render_flag_t (*ui_page_event_cb)(void* arg);
typedef void (*ui_page_render_cb)(u8g2_t* display, void* arg);
typedef void (*ui_page_lifecycle_cb)(void* arg);
typedef ui_render_flag_t (*ui_page_encoder_cb)(int difference, void* arg);
typedef ui_render_flag_t (*ui_page_button_cb
)(eom_hal_button_t button, eom_hal_button_event_t evt, void* arg);

struct ui_page {
    char title[UI_PAGE_TITLE_MAX];
    ui_page_lifecycle_cb on_open;
    ui_page_render_cb on_render;
    ui_page_event_cb on_loop;
    ui_page_lifecycle_cb on_close;
    ui_page_button_cb on_button;
    ui_page_encoder_cb on_encoder;
};

/***
 * Lifecycle methods.
 */

ui_render_flag_t ui_page_handle_button(
    const ui_page_t* p, eom_hal_button_t button, eom_hal_button_event_t event, void* arg
);

ui_render_flag_t ui_page_handle_encoder(const ui_page_t* p, int delta, void* arg);
void ui_page_handle_open(const ui_page_t* p, void* arg);
ui_render_flag_t ui_page_handle_loop(const ui_page_t* p, void* arg);
void ui_page_handle_render(const ui_page_t* p, u8g2_t* d, void* arg);
void ui_page_handle_close(const ui_page_t* p, void* arg);

/**
 * Register your pages here.
 */

// TODO: Move to pages/index.h and put page suffix.

extern const ui_page_t PAGE_SNAKE;
extern const ui_page_t PAGE_EDGING_STATS;
extern const ui_page_t PAGE_APP_RUNNER;

#ifdef __cplusplus
}
#endif

#endif
