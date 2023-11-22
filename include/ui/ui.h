#ifndef __ui__ui_h
#define __ui__ui_h

#ifdef __cplusplus
extern "C" {
#endif

// @todo At some point, we can possibly expand the history to a linked list of menu states/args.
#define UI_MENU_HISTORY_DEPTH 10

#include "ui/input.h"
#include "ui/menu.h"
#include "ui/page.h"

void ui_init(void);
void ui_tick(void);
void ui_redraw_all(void);
void ui_send_buffer(void);

void ui_open_page(const ui_page_t* page, void* arg);

void ui_open_menu(const ui_menu_t* menu, void* arg);
void ui_reenter_menu(void);
void ui_close_menu(void);
void ui_close_all_menu(void);

void ui_open_input(const ui_input_t* input, void* arg);
void ui_close_input(void);

void ui_close_all(void);
void ui_reset_idle_timer(void);
void ui_fade_to(uint8_t color);

#ifdef __cplusplus
}
#endif

#endif
