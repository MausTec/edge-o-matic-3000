#include "ui/menu.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "util/i18n.h"
#include <stdlib.h>
#include <string.h>

static const char* TAG = "ui:menu";

static size_t _index = 0;
static size_t _selected_char_idx = 0;
static SemaphoreHandle_t _insert_mutex = NULL;

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
    ui_open_input(input, (void*)item);
}

void ui_menu_cb_render_input(
    const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE menu_arg
) {
    if (item == NULL || item->arg == NULL) return;
    const ui_input_t* input = (const ui_input_t*)item->arg;

    if (input->type == INPUT_TYPE_TOGGLE) {
        const ui_input_toggle_t* i = (const ui_input_toggle_t*)input;
        char* label = (char*)item->label;

        if (*i->value)
            label[0] = MENU_ICON_CHECKBOX_ON;
        else
            label[0] = MENU_ICON_CHECKBOX_OFF;
    }
}

void ui_menu_cb_input_help(
    const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE menu_arg
) {
    if (item == NULL || item->arg == NULL) return;
    const ui_input_t* input = (const ui_input_t*)item->arg;
    ui_toast_multiline(input->help);
}

// Dynamic menu manipulation
ui_menu_item_node_t*
ui_menu_add_node(const ui_menu_t* m, const ui_menu_item_t* item, UI_MENU_ARG_TYPE arg) {
    if (m == NULL) return NULL;

    ui_menu_item_list_t* list = m->dynamic_items;
    if (list == NULL) return NULL;

    ui_menu_item_node_t* node = (ui_menu_item_node_t*)malloc(sizeof(ui_menu_item_node_t));
    if (node == NULL) return NULL;

    if (xSemaphoreTake(_insert_mutex, portMAX_DELAY) != pdTRUE) return NULL;

    node->item = item;
    node->next = NULL;
    node->freer = NULL;

    if (list->first == NULL) {
        list->first = node;
    } else {
        ui_menu_item_node_t* ptr = list->first;
        while (ptr != NULL && ptr->next != NULL) {
            ptr = ptr->next;
        }
        if (ptr != NULL) {
            ptr->next = node;
        }
    }

    list->count++;

    xSemaphoreGive(_insert_mutex);
    return node;
}

void ui_menu_clear_at(const ui_menu_t* m, size_t n) {
    if (m == NULL) return;
    ESP_LOGD(TAG, "ui_menu_clear(\"%s\")", m->title);

    ui_menu_item_list_t* list = m->dynamic_items;
    ui_menu_item_node_t** ptr = &list->first;
    if (list == NULL) return;

    while (*ptr != NULL) {
        // advance to offset:
        if (n > 0) {
            ptr = &(*ptr)->next;
            n--;
            continue;
        }

        ui_menu_item_node_t* node = *ptr;

        if (node->item && node->item->freer != NULL) {
            ESP_LOGD(TAG, "node->item->freer(%p) \"%s\"", node->item->arg, (char*)node->item->arg);
            node->item->freer(node->item->arg);
        }

        *ptr = node->next;
        if (node->freer != NULL) {
            ESP_LOGD(TAG, "node->freer(%p)", node->item);
            node->freer((void*)node->item);
        }

        ESP_LOGD(TAG, "free(%p)", node);
        free(node);
        list->count--;
    }

    ESP_LOGD(TAG, "Menu cleared.");
}

void ui_menu_clear(const ui_menu_t* m) {
    ui_menu_clear_at(m, 0);
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
    item->flags.translate_title = 0;
    item->on_select = on_select;
    item->on_option = NULL;
    item->on_render = NULL;
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
    const char* input_title = input->flags.translate_title ? _(input->title) : input->title;
    char title[UI_MENU_TITLE_MAX];

    if (input->type == INPUT_TYPE_TOGGLE) {
        ui_input_toggle_t* i = (ui_input_toggle_t*)input;
        snprintf(
            title,
            UI_MENU_TITLE_MAX,
            "%c %s",
            *i->value ? MENU_ICON_CHECKBOX_ON : MENU_ICON_CHECKBOX_OFF,
            input_title
        );

    } else {
        strncpy(title, input_title, UI_MENU_TITLE_MAX);
    }

    ui_menu_item_t* item = ui_menu_add_item(m, title, ui_menu_cb_open_input, (void*)input);

    strncpy(item->select_str, _("EDIT"), UI_BUTTON_STR_MAX);

    if (input->type == INPUT_TYPE_TOGGLE) {
        item->on_render = ui_menu_cb_render_input;
        // here we could also add a toggle button instead of opening to toggle
    }

    if (input->help != NULL) {
        item->on_option = ui_menu_cb_input_help;
        strlcpy(item->option_str, _("HELP"), UI_BUTTON_STR_MAX);
    }

    return item;
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
            ui_close_all();
            return RENDER;
        }
    }

    const ui_menu_item_t* item = ui_menu_get_nth_item(m, _index);
    if (item == NULL) return PASS;

    if (button == EOM_HAL_BUTTON_MID) {
        if (event == EOM_HAL_BUTTON_PRESS) {
            if (item->on_option != NULL) {
                item->on_option(m, item, arg);
                return RENDER;
            }
        }
    } else if (button == EOM_HAL_BUTTON_OK || button == EOM_HAL_BUTTON_MENU) {
        if (event == EOM_HAL_BUTTON_PRESS) {
            if (item->on_select != NULL) {
                item->on_select(m, item, arg);
                return RENDER;
            } else {
                ui_toast("%s", _("Option unavailable.\nPlease see manual."));
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

    if (_index != old_idx) {
        _selected_char_idx = 0;
        return RENDER;
    }

    return NORENDER;
}

void ui_menu_add_static_items(const ui_menu_t* m) {
}

void ui_menu_handle_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    if (_insert_mutex == NULL) {
        _insert_mutex = xSemaphoreCreateMutex();
    }

    if (m == NULL) return;

    _index = 0;
    _selected_char_idx = 0;

    ui_menu_add_static_items(m);

    if (m->on_open != NULL) {
        m->on_open(m, arg);
    }
}

void ui_menu_set_idx(size_t idx) {
    _index = idx;
}

size_t ui_menu_get_idx(void) {
    return _index;
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

void ui_menu_remove_item_at(const ui_menu_t* m, size_t idx) {
    if (m == NULL || m->dynamic_items == NULL) return;
    ESP_LOGD(TAG, "ui_menu_remove_item_at()");

    size_t i = 0;
    ui_menu_item_node_t* prev = NULL;
    ui_menu_item_node_t* node = m->dynamic_items->first;

    while (i < idx && node != NULL) {
        i++;
        prev = node;
        node = node->next;
    }

    ESP_LOGD(TAG, "-> node = %p", node);

    if (node == NULL) return;
    ESP_LOGD(TAG, "-> label = %s", node->item->label);

    m->dynamic_items->count--;

    if (node == m->dynamic_items->first) {
        m->dynamic_items->first = node->next;
    }

    if (prev != NULL) {
        prev->next = node->next;
    }

    if (node->item != NULL && node->item->freer != NULL) {
        ESP_LOGD(
            TAG,
            "node->item->freer<%p>(%p) \"%s\"",
            node->item->freer,
            node->item->arg,
            (char*)node->item->arg
        );
        node->item->freer(node->item->arg);
    }

    if (node->freer != NULL) {
        ESP_LOGD(TAG, "node->freer<%p>(%p)", node->freer, node->item);
        node->freer((void*)node->item);
    }

    ESP_LOGD(TAG, "free(%p)", node);
    free(node);
}

void ui_menu_add_item_at(const ui_menu_t* m, size_t idx, const ui_menu_item_t* item) {
    if (m == NULL || m->dynamic_items == NULL || item == NULL) return;
    ESP_LOGD(TAG, "ui_menu_add_item_at(%s)", item->label);

    size_t i = 0;
    ui_menu_item_node_t* prev = NULL;
    ui_menu_item_node_t* next = m->dynamic_items->first;

    while (next != NULL && next->next != NULL && i < idx) {
        prev = next;
        next = next->next;
    }

    ui_menu_item_node_t* node = (ui_menu_item_node_t*)malloc(sizeof(ui_menu_item_node_t));
    if (node == NULL) return;

    node->item = item;
    node->freer = NULL;
    node->next = next;

    if (prev != NULL) {
        prev->next = node;
    } else {
        m->dynamic_items->first = node;
    }

    m->dynamic_items->count++;
}

void ui_menu_replace_item_at(const ui_menu_t* m, size_t idx, const ui_menu_item_t* item) {
    if (xSemaphoreTake(_insert_mutex, portMAX_DELAY) != pdTRUE) return;
    ui_menu_remove_item_at(m, idx);
    ui_menu_add_item_at(m, idx, item);
    xSemaphoreGive(_insert_mutex);
}

const ui_menu_item_t* ui_menu_get_current_item(const ui_menu_t* m) {
    return ui_menu_get_nth_item(m, _index);
}

ui_render_flag_t ui_menu_handle_loop(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
    static unsigned long last_scroll_ms = 0;
    unsigned long millis = esp_timer_get_time() / 1000UL;

    if (millis - last_scroll_ms > 1000UL / 10) {
        last_scroll_ms = millis;
        _selected_char_idx++;
        return RENDER;
    }

    return PASS;
}

struct _menu_item_render_flags {
    uint8_t item_selected : 1;
    uint8_t item_disabled : 1;
};

static void
_render_menu_item(u8g2_t* d, uint8_t y, const char* label, struct _menu_item_render_flags flags) {
    if (y > 5) return;

    u8g2_SetFont(d, UI_FONT_DEFAULT);
    u8g2_SetFontPosTop(d);
    u8g2_SetDrawColor(d, 1);

    char label_prefix = 0x00;

    if (label[0] > 0x00 && label[0] < 0x10) {
        label_prefix = label[0];
        label++;
    }

    uint8_t text_x = 1;
    uint8_t text_y = (10 * y) + 1;

    if (label_prefix > 0x00) {
        text_x = 8;
    }

    if (flags.item_selected) {
        u8g2_DrawBox(d, 0, 10 + text_y, EOM_DISPLAY_WIDTH - 3, 9);
        u8g2_SetDrawColor(d, 0);

        // Handle scrolling thing:
        size_t label_len = strlen(label);

        if (label_len < 20) {
            u8g2_DrawUTF8(d, text_x, 10 + text_y, label);
        } else {
            size_t offset = (_selected_char_idx / 6) % (label_len);
            u8g2_DrawUTF8(d, text_x + (6 - (_selected_char_idx % 6)), 10 + text_y, label + offset);
            u8g2_SetDrawColor(d, 1);
            u8g2_DrawBox(d, text_x - 1, 10 + text_y, 7, 9);
            u8g2_SetDrawColor(d, 0);
            u8g2_DrawGlyph(d, text_x, 10 + text_y, '<');
        }
    } else {
        u8g2_DrawUTF8(d, text_x, 10 + text_y, label);
    }

    if (flags.item_disabled) {
        if (flags.item_selected) u8g2_SetDrawColor(d, 0);
        // ui_draw_shaded_rect(d, 0, 10 + text_y, EOM_DISPLAY_WIDTH - 3, 9, 0);
        int line_x2 = u8g2_GetUTF8Width(d, label) + text_x;
        if (line_x2 > EOM_DISPLAY_WIDTH - 3) line_x2 = EOM_DISPLAY_WIDTH - 3;
        u8g2_DrawLine(d, text_x + 4, 14 + text_y, line_x2, 14 + text_y);
    }

    u8g2_SetFont(d, UI_FONT_SMALL);

    switch (label_prefix) {
    case MENU_ICON_CHECKBOX_ON:
        u8g2_DrawLine(d, 2, 12 + text_y, 6, 16 + text_y);
        u8g2_DrawLine(d, 2, 16 + text_y, 6, 12 + text_y);
        // fall through
    case MENU_ICON_CHECKBOX_OFF: u8g2_DrawFrame(d, 1, 11 + text_y, 7, 7); break;
    default: break;
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
            if (idx >= offset) {
                const ui_menu_item_t* item = node->item;
                struct _menu_item_render_flags flags = {
                    .item_disabled = item->on_select == NULL ? 1 : 0,
                    .item_selected = 0,
                };

                if (item->on_render != NULL) item->on_render(m, item, arg);

                if (idx == _index) {
                    flags.item_selected = 1;
                    current_item = item;
                }

                if ((idx - offset) > 5) break;
                _render_menu_item(d, idx - offset, item->label, flags);
            }

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