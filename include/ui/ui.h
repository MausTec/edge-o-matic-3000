#ifndef __ui__ui_h
#define __ui__ui_h

#ifdef __cplusplus
extern "C" {
#endif

#include "ui/page.h"
#include "ui/menu.h"
#include "ui/input.h"

void ui_init(void);
void ui_tick(void);

void ui_open_page(ui_page_t *page);

void ui_open_menu(ui_menu_t *menu);
void ui_close_menu(void);

void ui_open_input(ui_input_t *input);
void ui_close_input(void);

#ifdef __cplusplus
}
#endif

#endif
