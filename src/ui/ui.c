#include "ui/ui.h"
#include "esp_log.h"
#include "ui/toast.h"
#include <string.h>

static const char* TAG = "UI";

static volatile struct ui_state {
    const ui_page_t* current_page;
    void* current_page_arg;

    const ui_menu_t* menu_history[UI_MENU_HISTORY_DEPTH];
    void* menu_arg_history[UI_MENU_HISTORY_DEPTH];
    size_t menu_history_depth;

    const ui_input_t* current_input;
    void* current_input_arg;

    ui_render_flag_t force_rerender;
} UI;

static uint8_t _initialized = 0;

static void handle_button(eom_hal_button_t button, eom_hal_button_event_t event) {
    ui_render_flag_t rf = PASS;

    if (ui_toast_is_active()) {
        if (ui_toast_is_dismissable()) {
            ui_toast_clear();
            UI.force_rerender = 1;
        }

        return;
    }

    if (UI.current_input != NULL) {
        const ui_input_t* i = UI.current_input;
        void* arg = UI.current_input_arg;
        rf = ui_input_handle_button(i, button, event, arg);
    }

    else if (UI.menu_history[UI.menu_history_depth] != NULL) {
        const ui_menu_t* m = UI.menu_history[UI.menu_history_depth];
        void* arg = UI.menu_arg_history[UI.menu_history_depth];
        rf = ui_menu_handle_button(m, button, event, arg);
    }

    else if (UI.current_page != NULL) {
        const ui_page_t* p = UI.current_page;
        void* arg = UI.current_page_arg;
        rf = ui_page_handle_button(p, button, event, arg);
    }

    if (rf == RENDER) {
        UI.force_rerender = 1;
    }
}

static void handle_encoder(int delta) {
    ui_render_flag_t rf = PASS;

    // Toasts always eat encoder...
    if (ui_toast_is_active()) return;

    if (UI.current_input != NULL) {
        const ui_input_t* i = UI.current_input;
        void* arg = UI.current_input_arg;
        rf = ui_input_handle_encoder(i, delta, arg);
    }

    else if (UI.menu_history[UI.menu_history_depth] != NULL) {
        const ui_menu_t* m = UI.menu_history[UI.menu_history_depth];
        void* arg = UI.menu_arg_history[UI.menu_history_depth];
        rf = ui_menu_handle_encoder(m, delta, arg);
    }

    else if (UI.current_page != NULL) {
        const ui_page_t* p = UI.current_page;
        void* arg = UI.current_page_arg;
        rf = ui_page_handle_encoder(p, delta, arg);
    }

    if (rf == RENDER) UI.force_rerender = 1;
}

void ui_init(void) {
    if (_initialized) return;

    // We can't simply memset here since that's, apparently, not volatile safe.
    UI.current_page = NULL;
    UI.current_page_arg = NULL;
    UI.menu_history_depth = 0;
    UI.current_input = NULL;
    UI.force_rerender = 0;

    for (int i = 0; i < UI_MENU_HISTORY_DEPTH; i++) {
        UI.menu_history[i] = NULL;
        UI.menu_arg_history[i] = NULL;
    }

    eom_hal_register_button_handler(handle_button);
    eom_hal_register_encoder_handler(handle_encoder);
    _initialized = true;
}

void ui_open_page(const ui_page_t* p, void* arg) {
    ui_page_handle_close(UI.current_page, UI.current_page_arg);

    UI.current_page = p;
    UI.current_page_arg = arg;

    if (p != NULL) {
        ui_page_handle_open(p, arg);
        UI.force_rerender = RENDER;
    }
}

void ui_open_input(const ui_input_t* i, void* arg) {
    ui_input_handle_close(i, arg);

    UI.current_input = i;
    UI.current_input_arg = arg;

    if (i != NULL) {
        ui_input_handle_open(i, arg);
        UI.force_rerender = RENDER;
    }
}

void ui_close_input(void) {
    if (UI.current_input == NULL) return;

    ui_input_handle_close(UI.current_input, UI.current_input_arg);
    UI.current_input = NULL;
    UI.current_input_arg = NULL;
    UI.force_rerender = RENDER;
}

void ui_open_menu(const ui_menu_t* menu, void* arg) {
    size_t new_idx = (UI.menu_history_depth + 1) % UI_MENU_HISTORY_DEPTH;
    size_t next_idx = (UI.menu_history_depth + 2) % UI_MENU_HISTORY_DEPTH;

    // Prevent infinite menu loops
    // todo - this throws away the arg pointer, which may not be what we want here since that could
    // lead to leaks, check how this is implemented. We may not call close until we go back or
    // dispose a menu. The problem, however, is that at a certain point a lower menu MAY hold onto
    // something a higher menu wants in an arg, so if we dispose it here, it'll break higher menus.
    UI.menu_arg_history[next_idx] = NULL;
    UI.menu_history[next_idx] = NULL;

    const ui_menu_t* current = UI.menu_history[UI.menu_history_depth];

    if (current != NULL) {
        ui_menu_handle_close(current, UI.menu_arg_history[UI.menu_history_depth]);
    }

    UI.menu_history[new_idx] = menu;
    UI.menu_arg_history[new_idx] = arg;
    UI.menu_history_depth = new_idx;
    UI.force_rerender = RENDER;

    ui_menu_handle_open(menu, arg);
}

// this will automatically go back, if it needs to.
void ui_close_menu(void) {
    size_t new_idx = (UI.menu_history_depth - 1) % UI_MENU_HISTORY_DEPTH;

    const ui_menu_t* current = UI.menu_history[UI.menu_history_depth];

    if (current != NULL) {
        ui_menu_handle_close(current, UI.menu_arg_history[UI.menu_history_depth]);
    }

    UI.menu_history[UI.menu_history_depth] = NULL;
    UI.menu_arg_history[UI.menu_history_depth] = NULL;
    UI.menu_history_depth = new_idx;
    UI.force_rerender = RENDER;

    const ui_menu_t* menu = UI.menu_history[UI.menu_history_depth];

    if (menu != NULL) {
        ui_menu_handle_open(menu, UI.menu_arg_history[UI.menu_history_depth]);
    }
}

void ui_close_all_menu(void) {
    while (UI.menu_history[UI.menu_history_depth] != NULL) {
        ui_close_menu();
    }
}

/**
 * Mersenne prime hash calculation to quickly detect if the buffer ever changed.
 */
static size_t calculate_hash(const uint8_t* buf, size_t len) {
    size_t hash = 2166136261U;
    for (int i = 0; i < len; i++) {
        hash = (127 * hash) + buf[i];
    }
    return hash;
}

void ui_send_buffer(void) {
    u8g2_t* display = eom_hal_get_display_ptr();
    size_t bufsz = 8 * u8g2_GetBufferTileHeight(display) * u8g2_GetBufferTileWidth(display);
    uint8_t* bufptr = u8g2_GetBufferPtr(display);
    static size_t last_hash = 0;

    // Render Common First
    ui_toast_render();

    size_t hash = calculate_hash(bufptr, bufsz);

    if (hash != last_hash) {
        u8g2_SendBuffer(display);
        last_hash = hash;
    }
}

void ui_tick(void) {
    int rendered = 0;
    u8g2_t* display = eom_hal_get_display_ptr();

    // Reset render conditions:
    u8g2_SetDrawColor(display, 1);
    u8g2_SetFont(display, UI_FONT_DEFAULT);

    if (UI.current_input != NULL) {
        const ui_input_t* i = UI.current_input;
        void* arg = UI.current_input_arg;

        ui_render_flag_t rf = ui_input_handle_loop(i, arg);
        if (UI.force_rerender == RENDER) rf = RENDER;

        if (!rendered) {
            if (rf == RENDER) {
                u8g2_ClearBuffer(display);

                if (UI.menu_history[UI.menu_history_depth] != NULL) {
                    ui_menu_handle_render(
                        UI.menu_history[UI.menu_history_depth],
                        display,
                        UI.menu_arg_history[UI.menu_history_depth]
                    );
                } else if (UI.current_page != NULL) {
                    ui_page_handle_render(UI.current_page, display, UI.current_page_arg);
                }

                ui_input_handle_render(i, display, arg);
                ui_send_buffer();
            }

            rendered = 1;
        }
    }

    if (UI.menu_history[UI.menu_history_depth] != NULL) {
        const ui_menu_t* m = UI.menu_history[UI.menu_history_depth];
        void* arg = UI.menu_arg_history[UI.menu_history_depth];

        ui_render_flag_t rf = ui_menu_handle_loop(m, arg);
        if (UI.force_rerender == RENDER) rf = RENDER;

        if (!rendered) {
            if (rf == RENDER) {
                u8g2_ClearBuffer(display);
                ui_menu_handle_render(m, display, arg);
                ui_send_buffer();
            }

            rendered = 1;
        }
    }

    if (UI.current_page != NULL) {
        const ui_page_t* p = UI.current_page;
        void* arg = UI.current_page_arg;

        // TODO: Swap pages to take an arg, not the current page.
        ui_render_flag_t rf = p->on_loop == NULL ? NORENDER : p->on_loop(arg);
        if (UI.force_rerender == RENDER) rf = RENDER;

        if (!rendered) {
            if (rf == RENDER && p->on_render != NULL) {
                u8g2_ClearBuffer(display);
                p->on_render(display, arg);
                ui_send_buffer();
            }

            rendered = 1;
        }
    }

    UI.force_rerender = NORENDER;
}

void ui_redraw_all(void) {
    UI.force_rerender = RENDER;
}