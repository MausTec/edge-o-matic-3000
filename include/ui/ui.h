#ifndef __ui__ui_h
#define __ui__ui_h

#ifdef __cplusplus
extern "C" {
#endif

#define MENU_HISTORY_DEPTH 10

#include "ui/page.h"
#include "ui/menu.h"
#include "ui/input.h"
#include "ui/render.h"
#include "ui/graphics.h"
#include "ui/toast.h"

void ui_init(void);
void ui_tick(void);

void ui_open_page(const ui_page_t *page, void *arg);

void ui_open_menu(const ui_menu_t *menu, void *arg);
void ui_close_menu(void);

void ui_open_input(const ui_input_t *input, void *arg);
void ui_close_input(void);

#ifdef __cplusplus
}
#endif

#endif
