#include "ui/menu.h"
#include <stdlib.h>

void ui_menu_cb_open_page(const ui_menu_t* m, void* arg);
void ui_menu_cb_open_menu(const ui_menu_t* m, void* arg);

// Dynamic menu manipulation
void ui_menu_add_node(const ui_menu_t* m, ui_menu_item_t* node, void* arg) {
}

ui_menu_item_t* ui_menu_add_item(
    const ui_menu_t* m, const char* label, ui_menu_item_callback_t on_select, void* arg
) {
    ui_menu_item_list_t* list = m->dynamic_items;
    if (list == NULL)
        return NULL;

    list->first = (ui_menu_item_t*)malloc(sizeof(ui_menu_item_t));
    return NULL;
}

ui_menu_item_t* ui_menu_add_page(const ui_menu_t* m, ui_page_t* page) {
    return NULL;
}

ui_menu_item_t* ui_menu_add_menu(const ui_menu_t* m, ui_menu_t* menu) {
    return NULL;
}

// Menu Lifecycle Callbacks
ui_render_flag_t ui_menu_handle_button(
    const ui_menu_t* m, eom_hal_button_t button, eom_hal_button_event_t event, void* arg
) {
    return PASS;
}

ui_render_flag_t ui_menu_handle_encoder(const ui_menu_t* m, int delta, void* arg) {
    return PASS;
}

void ui_menu_handle_open(const ui_menu_t* m, void* arg) {
}

ui_render_flag_t ui_menu_handle_loop(const ui_menu_t* m, void* arg) {
    return PASS;
}

void ui_menu_handle_render(const ui_menu_t* m, u8g2_t* d, void* arg) {
}

void ui_menu_handle_close(const ui_menu_t* m, void* arg) {
}