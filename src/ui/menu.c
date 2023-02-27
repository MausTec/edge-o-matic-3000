#include "ui/menu.h"
#include "esp_log.h"
#include "ui/ui.h"
#include "util/i18n.h"
#include <stdlib.h>
#include <string.h>

static const char* TAG = "ui:menu";
static size_t _index = 0;

void ui_menu_cb_open_page(
    const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE menu_arg
) {
}

void ui_menu_cb_open_menu(
    const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE menu_arg
) {
    if (item == NULL) return;
    const ui_menu_t* menu = (const ui_menu_t*)item->arg;
    ui_open_menu(menu, NULL);
}

void ui_menu_cb_open_input(
    const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE menu_arg
) {
    if (item == NULL) return;
    const ui_input_t* input = (const ui_input_t*)item->arg;
    ui_open_input(input, NULL);
}

// Dynamic menu manipulation
ui_menu_item_node_t*
ui_menu_add_node(const ui_menu_t* m, ui_menu_item_t* item, UI_MENU_ARG_TYPE arg) {
    if (m == NULL) return NULL;

    ui_menu_item_list_t* list = m->dynamic_items;
    if (list == NULL) return NULL;

    ui_menu_item_node_t* node = (ui_menu_item_node_t*)malloc(sizeof(ui_menu_item_node_t));
    if (node == NULL) return NULL;

    node->item = item;
    node->next = NULL;

    if (list->first == NULL) {
        list->first = node;
        list->last = node;
    } else {
        list->last->next = node;
        list->last = node;
    }

    list->count++;
    return node;
}

void ui_menu_clear(const ui_menu_t* m) {
    if (m == NULL) return;
    ESP_LOGD(TAG, "ui_menu_clear(\"%s\")", m->title);

    ui_menu_item_list_t* list = m->dynamic_items;
    if (list == NULL) return;

    while (list->first != NULL) {
        ui_menu_item_node_t* node = list->first;

        if (node->item && node->item->freer != NULL) {
            ESP_LOGD(TAG, "node->item->freer(%p) \"%s\"", node->item->arg, (char*)node->item->arg);
            node->item->freer(node->item->arg);
            node->item->arg = NULL;
        }

        list->first = node->next;
        if (node->freer != NULL) {
            ESP_LOGD(TAG, "node->freer(%p)", node->item);
            node->freer(node->item);
        }

        ESP_LOGD(TAG, "free(%p)", node);
        free(node);
    }

    list->last = NULL;
    list->count = 0;
    ESP_LOGD(TAG, "Menu cleared.");
}

void ui_menu_handle_close(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    if (m == NULL) return;

    if (m->on_close != NULL) {
        m->on_close(m, arg);
    }

    ui_menu_clear(m);
}

ui_menu_item_t* ui_menu_add_item(
    const ui_menu_t* m, const char* label, ui_menu_item_callback_t on_select, UI_MENU_ARG_TYPE arg
) {
    if (m == NULL) return NULL;

    ui_menu_item_t* item = (ui_menu_item_t*)malloc(sizeof(ui_menu_item_t));
    if (item == NULL) return NULL;

    strncpy(item->label, label, UI_MENU_TITLE_MAX);
    item->arg = arg;
    item->on_select = on_select;
    item->on_option = NULL;
    item->option_str[0] = '\0';
    item->select_str[0] = '\0';
    item->freer = NULL;

    ui_menu_item_node_t* node = ui_menu_add_node(m, item, arg);

    if (node == NULL) {
        free(item);
        return NULL;
    }

    node->freer = free;
    return item;
}

ui_menu_item_t* ui_menu_add_page(const ui_menu_t* m, const ui_page_t* page) {
    return NULL;
}

ui_menu_item_t* ui_menu_add_menu(const ui_menu_t* m, const ui_menu_t* menu) {
    if (m == NULL || menu == NULL) return NULL;

    return ui_menu_add_item(
        m,
        menu->flags.translate_title ? _(menu->title) : menu->title,
        ui_menu_cb_open_menu,
        (void*)menu
    );
}

ui_menu_item_t* ui_menu_add_input(const ui_menu_t* m, const ui_input_t* input) {
    if (m == NULL || input == NULL) return NULL;

    return ui_menu_add_item(
        m,
        input->flags.translate_title ? _(input->title) : input->title,
        ui_menu_cb_open_input,
        (void*)input
    );
}

// Menu Lifecycle Callbacks
ui_render_flag_t ui_menu_handle_button(
    const ui_menu_t* m, eom_hal_button_t button, eom_hal_button_event_t event, UI_MENU_ARG_TYPE arg
) {
    if (button == EOM_HAL_BUTTON_BACK) {
        if (event == EOM_HAL_BUTTON_PRESS) {
            ui_close_menu();
            return RENDER;
        } else if (event == EOM_HAL_BUTTON_HOLD) {
            ui_close_all_menu();
            return RENDER;
        }
    }

    const ui_menu_item_t* item = ui_menu_get_nth_item(m, _index);
    if (item == NULL) return PASS;

    if (button == EOM_HAL_BUTTON_MID) {
        // menu button event
    } else if (button == EOM_HAL_BUTTON_OK || button == EOM_HAL_BUTTON_MENU) {
        if (event == EOM_HAL_BUTTON_PRESS) {
            if (item->on_select != NULL) {
                item->on_select(m, item, arg);
                return RENDER;
            }
        }
    }

    return PASS;
}

ui_render_flag_t ui_menu_handle_encoder(const ui_menu_t* m, int delta, UI_MENU_ARG_TYPE arg) {
    if (m == NULL) return PASS;
    if (m->dynamic_items == NULL) return NORENDER;

    // disable menu acceleration
    delta = delta > 0 ? 1 : delta == 0 ? 0 : -1;

    size_t old_idx = _index;
    int max = m->dynamic_items->count - 1;
    int new_idx = _index + delta;

    if (new_idx < 0)
        _index = 0;
    else if (new_idx > max)
        _index = max;
    else
        _index = new_idx;

    return _index == old_idx ? NORENDER : RENDER;
}

void ui_menu_add_static_items(const ui_menu_t* m) {
}

void ui_menu_handle_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    if (m == NULL) return;
    _index = 0;

    ui_menu_add_static_items(m);

    if (m->on_open != NULL) {
        m->on_open(m, arg);
    }
}

const ui_menu_item_t* ui_menu_get_nth_item(const ui_menu_t* m, size_t n) {
    if (m == NULL) return NULL;

    if (m->dynamic_items != NULL) {
        uint8_t idx = 0;
        ui_menu_item_node_t* node = m->dynamic_items->first;

        while (node != NULL) {
            if (idx++ == n) return node->item;
            node = node->next;
        }
    }

    return NULL;
}

const ui_menu_item_t* ui_menu_get_current_item(const ui_menu_t* m) {
    return ui_menu_get_nth_item(m, _index);
}

ui_render_flag_t ui_menu_handle_loop(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    return PASS;
}

struct _menu_item_render_flags {
    uint8_t item_selected : 1;
    uint8_t item_disabled : 1;
};

static void
_render_menu_item(u8g2_t* d, uint8_t y, const char* label, struct _menu_item_render_flags flags) {
    if (y < 0 || y > 5) return;

    u8g2_SetFont(d, UI_FONT_DEFAULT);
    u8g2_SetFontPosTop(d);
    u8g2_SetDrawColor(d, 1);

    if (flags.item_selected) {
        u8g2_DrawBox(d, 0, 10 + (10 * y), EOM_DISPLAY_WIDTH - 3, 10);
        u8g2_SetDrawColor(d, 0);
    }

    u8g2_DrawUTF8(d, 1, 11 + (10 * y), label);

    if (flags.item_disabled && !flags.item_selected) {
        u8g2_SetDrawColor(d, 0);
        ui_draw_shaded_rect(d, 0, 10 + (10 * y), EOM_DISPLAY_WIDTH - 3, 10, 0);
    }
}

void ui_menu_handle_render(const ui_menu_t* m, u8g2_t* d, UI_MENU_ARG_TYPE arg) {
    if (m == NULL) return;
    const ui_menu_item_t* current_item = NULL;

    ui_draw_status(d, m->title);

    u8g2_SetDrawColor(d, 1);
    u8g2_DrawHLine(d, 0, 9, EOM_DISPLAY_WIDTH);

    // Menu items
    if (m->dynamic_items != NULL) {
        size_t idx = 0;
        size_t offset = (_index > 2 ? _index - 2 : 0);
        ui_menu_item_node_t* node = m->dynamic_items->first;

        while (node != NULL) {
            ui_menu_item_t* item = node->item;
            struct _menu_item_render_flags flags = { .item_disabled =
                                                         item->on_select == NULL ? 1 : 0,
                                                     .item_selected = 0 };

            if (idx == _index) {
                flags.item_selected = 1;
                current_item = item;
            }

            _render_menu_item(d, idx - offset, item->label, flags);
            idx++;
            node = node->next;
        }

        ui_draw_scrollbar(d, _index, m->dynamic_items->count, 5);
    }

    ui_draw_button_labels(
        d,
        _("BACK"),
        current_item != NULL && current_item->option_str[0] != '\0' ? current_item->option_str : "",
        current_item != NULL && current_item->select_str[0] != '\0' ? current_item->select_str
                                                                    : _("ENTER")
    );

    ui_draw_button_disable(
        d,
        0x00 | (current_item == NULL || current_item->on_option == NULL ? 0b10 : 0x00) |
            (current_item == NULL || current_item->on_select == NULL ? 0b01 : 0x00)
    );
}