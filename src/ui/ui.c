#include "ui/ui.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "UI";

static volatile struct ui_state {
    const ui_page_t* current_page;
    void* current_page_arg;

    const ui_menu_t* menu_history[MENU_HISTORY_DEPTH];
    void* menu_arg_history[MENU_HISTORY_DEPTH];
    size_t menu_history_depth;

    const ui_input_t* current_input;
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
    }

    else if (UI.menu_history_depth > 0) {
        const ui_menu_t* m = UI.menu_history[UI.menu_history_depth - 1];
        if (m == NULL)
            return; // This is an invalid state.
        void* arg = UI.menu_arg_history[UI.menu_history_depth - 1];
        // if (m->on_button != NULL) rf = m->on_button(button, event, arg);
    }

    else if (UI.current_page != NULL) {
        const ui_page_t* p = UI.current_page;
        void* arg = UI.current_page_arg;
        if (p->on_button != NULL)
            rf = p->on_button(button, event, arg);
    }

    if (rf == RENDER)
        UI.force_rerender = 1;
}

static void handle_encoder(int delta) {
    ui_render_flag_t rf = PASS;

    // Toasts always eat encoder...
    if (ui_toast_is_active())
        return;

    if (UI.current_input != NULL) {
    }

    else if (UI.menu_history_depth > 0) {
        const ui_menu_t* m = UI.menu_history[UI.menu_history_depth - 1];
        if (m == NULL)
            return; // This is an invalid state.
        void* arg = UI.menu_arg_history[UI.menu_history_depth - 1];
        // if (m->on_encoder != NULL) rf = m->on_encoder(delta, arg);
    }

    else if (UI.current_page != NULL) {
        const ui_page_t* p = UI.current_page;
        void* arg = UI.current_page_arg;
        if (p->on_encoder != NULL)
            rf = p->on_encoder(delta, arg);
    }

    if (rf == RENDER)
        UI.force_rerender = 1;
}

void ui_init(void) {
    if (_initialized)
        return;

    // We can't simply memset here since that's, apparently, not volatile safe.
    UI.current_page = NULL;
    UI.current_page_arg = NULL;
    UI.menu_history_depth = 0;
    UI.current_input = NULL;
    UI.force_rerender = 0;

    for (int i = 0; i < MENU_HISTORY_DEPTH; i++) {
        UI.menu_history[i] = NULL;
        UI.menu_arg_history[i] = NULL;
    }

    eom_hal_register_button_handler(handle_button);
    eom_hal_register_encoder_handler(handle_encoder);
    _initialized = true;
}

void ui_open_page(const ui_page_t* p, void* arg) {
    if (UI.current_page != NULL) {
        UI.current_page->on_close(UI.current_page_arg);
        // Dispose current page arg?
    }

    UI.current_page = p;
    UI.current_page_arg = arg;

    if (p != NULL) {
        ESP_LOGI(TAG, "Open page \"%s\"", p->title);
        if (p->on_open)
            p->on_open(arg);
        UI.force_rerender = RENDER;
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
        rendered = 1;
    }

    if (UI.menu_history_depth > 0) {
        const ui_menu_t* m = UI.menu_history[UI.menu_history_depth - 1];
        if (m == NULL)
            return; // This is an invalid state.

        ui_render_flag_t rf = NORENDER;
        if (UI.force_rerender == RENDER)
            rf = RENDER;

        if (!rendered) {
            void* arg = UI.menu_arg_history[UI.menu_history_depth - 1];
            // if (rf == RENDER && m->on_render != NULL) m->on_render(display, arg);
            rendered = 1;
        }
    }

    if (UI.current_page != NULL) {
        const ui_page_t* p = UI.current_page;
        void* arg = UI.current_page_arg;

        // TODO: Swap pages to take an arg, not the current page.
        ui_render_flag_t rf = p->on_loop == NULL ? NORENDER : p->on_loop(arg);
        if (UI.force_rerender == RENDER)
            rf = RENDER;

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