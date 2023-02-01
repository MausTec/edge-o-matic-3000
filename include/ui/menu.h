#ifndef __ui__menu_h
#define __ui__menu_h

#ifdef __cplusplus
extern "C" {
#endif

#include "ui/graphics.h"
#include "ui/page.h"

#define UI_MENU_TITLE_MAX 64

struct ui_menu;

typedef void (*ui_menu_item_callback_t)(const struct ui_menu* m, void* arg);
typedef void (*ui_menu_open_callback_t)(const struct ui_menu* m, void* arg);

typedef struct ui_menu_item {
    char label[UI_MENU_TITLE_MAX];
    void* arg;
    char select_str[UI_BUTTON_STR_MAX];
    char option_str[UI_BUTTON_STR_MAX];
    ui_menu_item_callback_t on_select;
    ui_menu_item_callback_t on_option;
    struct ui_menu_item* next;
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

void ui_menu_cb_open_page(const ui_menu_t* m, void* arg);
void ui_menu_cb_open_menu(const ui_menu_t* m, void* arg);

// Dynamic menu manipulation
void ui_menu_add_node(const ui_menu_t* m, ui_menu_item_t* node, void* arg);
ui_menu_item_t* ui_menu_add_item(
    const ui_menu_t* m, const char* label, ui_menu_item_callback_t on_select, void* arg
);
ui_menu_item_t* ui_menu_add_page(const ui_menu_t* m, ui_page_t* page);
ui_menu_item_t* ui_menu_add_menu(const ui_menu_t* m, ui_menu_t* menu);

// Menu Lifecycle Callbacks
ui_render_flag_t ui_menu_handle_button(
    const ui_menu_t* m, eom_hal_button_t button, eom_hal_button_event_t event, void* arg
);

ui_render_flag_t ui_menu_handle_encoder(const ui_menu_t* m, int delta, void* arg);
void ui_menu_handle_open(const ui_menu_t* m, void* arg);
ui_render_flag_t ui_menu_handle_loop(const ui_menu_t* m, void* arg);
void ui_menu_handle_render(const ui_menu_t* m, u8g2_t* d, void* arg);
void ui_menu_handle_close(const ui_menu_t* m, void* arg);

/**
 * Register your menus here.
 */

extern const ui_menu_t MAIN_MENU;

#ifdef __cplusplus
}
#endif

#endif
