#ifndef __ui__menu_h
#define __ui__menu_h

#ifdef __cplusplus
extern "C" {
#endif

#include "ui/graphics.h"
#include "ui/page.h"

#define UI_MENU_TITLE_MAX 64
#define UI_MENU_ARG_TYPE const void*

struct ui_menu;
struct ui_menu_item;

typedef void (*ui_menu_item_callback_t
)(const struct ui_menu* m, const struct ui_menu_item* item, UI_MENU_ARG_TYPE menu_arg);
typedef void (*ui_menu_open_callback_t)(const struct ui_menu* m, UI_MENU_ARG_TYPE arg);
typedef void (*ui_menu_item_free_callback_t)(void* arg);

typedef struct ui_menu_item {
    char label[UI_MENU_TITLE_MAX];
    UI_MENU_ARG_TYPE arg;
    char select_str[UI_BUTTON_STR_MAX];
    char option_str[UI_BUTTON_STR_MAX];
    ui_menu_item_callback_t on_select;
    ui_menu_item_callback_t on_option;
} ui_menu_item_t;

typedef struct ui_menu_item_node {
    ui_menu_item_t* item;
    struct ui_menu_item_node* next;
} ui_menu_item_node_t;

typedef struct ui_menu_item_list {
    ui_menu_item_node_t* first;
    ui_menu_item_node_t* last;
    size_t count;
} ui_menu_item_list_t;

typedef struct ui_menu {
    char title[UI_MENU_TITLE_MAX];
    ui_menu_open_callback_t on_open;
    ui_menu_open_callback_t on_close;
    ui_menu_item_list_t* dynamic_items;
    ui_menu_item_t static_items[];
} ui_menu_t;

#define STATIC_MENU(title_str, items)                                                              \
    {                                                                                              \
        .title = title_str, .on_open = NULL, .on_close = NULL, .dynamic_items = NULL,              \
        .static_items = items,                                                                     \
    }

#define DYNAMIC_MENU(symbol, title_str, open_cb)                                                   \
    static ui_menu_item_list_t container = { .first = NULL, .last = NULL, .count = 0 };            \
    const ui_menu_t symbol = { .title = title_str,                                                 \
                               .on_open = open_cb,                                                 \
                               .on_close = NULL,                                                   \
                               .dynamic_items = &container,                                        \
                               .static_items = {} }

void ui_menu_cb_open_page(
    const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE menu_arg
);
void ui_menu_cb_open_menu(
    const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE menu_arg
);

// Dynamic menu manipulation
void ui_menu_add_static_items(const ui_menu_t* m);
int ui_menu_add_node(const ui_menu_t* m, ui_menu_item_t* node, UI_MENU_ARG_TYPE arg);
ui_menu_item_t* ui_menu_add_item(
    const ui_menu_t* m, const char* label, ui_menu_item_callback_t on_select, UI_MENU_ARG_TYPE arg
);
ui_menu_item_t* ui_menu_add_page(const ui_menu_t* m, ui_page_t* page);
ui_menu_item_t* ui_menu_add_menu(const ui_menu_t* m, ui_menu_t* menu);
void ui_menu_clear(const ui_menu_t* m);
void ui_menu_free(const ui_menu_t* m, ui_menu_item_free_callback_t free_cb);

const ui_menu_item_t* ui_menu_get_nth_item(const ui_menu_t* m, size_t n);
const ui_menu_item_t* ui_menu_get_current_item(const ui_menu_t* m);

// Menu Lifecycle Callbacks
ui_render_flag_t ui_menu_handle_button(
    const ui_menu_t* m, eom_hal_button_t button, eom_hal_button_event_t event, UI_MENU_ARG_TYPE arg
);

ui_render_flag_t ui_menu_handle_encoder(const ui_menu_t* m, int delta, UI_MENU_ARG_TYPE arg);
void ui_menu_handle_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg);
ui_render_flag_t ui_menu_handle_loop(const ui_menu_t* m, UI_MENU_ARG_TYPE arg);
void ui_menu_handle_render(const ui_menu_t* m, u8g2_t* d, UI_MENU_ARG_TYPE arg);
void ui_menu_handle_close(const ui_menu_t* m, UI_MENU_ARG_TYPE arg);

#ifdef __cplusplus
}
#endif

#endif
