#include "ui/ui.h"
#include "assets.h"
#include "config.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "orgasm_control.h"
#include "ui/graphics.h"
#include "ui/screenshot.h"
#include "ui/toast.h"
#include "util/i18n.h"
#include <string.h>

static const char* TAG = "UI";

static volatile struct ui_state {
    const ui_page_t* current_page;
    void* current_page_arg;

    const ui_menu_t* menu_history[UI_MENU_HISTORY_DEPTH];
    void* menu_arg_history[UI_MENU_HISTORY_DEPTH];
    size_t menu_idx_history[UI_MENU_HISTORY_DEPTH];
    size_t menu_history_depth;

    const ui_input_t* current_input;
    void* current_input_arg;

    ui_render_flag_t force_rerender;
    uint64_t last_input_ms;

    enum {
        UI_ACTIVE,
        UI_IDLE,
        UI_SCREENSAVER,
    } idle_state;
} UI;

static uint8_t _initialized = 0;

void ui_reset_idle_timer(void) {
    UI.last_input_ms = esp_timer_get_time() / 1000UL;
    u8g2_t* display = eom_hal_get_display_ptr();

    if (UI.idle_state != UI_ACTIVE) {
        u8g2_SetContrast(display, 255);
        UI.idle_state = UI_ACTIVE;
        ui_tick();
    }
}

static void handle_button(eom_hal_button_t button, eom_hal_button_event_t event) {
    ui_render_flag_t rf = PASS;
    ui_reset_idle_timer();
    u8g2_t* display = eom_hal_get_display_ptr();

    // Handle Screenshots / Debug Control (Menu + Other)
    if (event == EOM_HAL_BUTTON_HOLD) {
        uint8_t down = eom_hal_get_button_state() & (~button);

        if (down) { // Multi-Button Combo
            if (button == EOM_HAL_BUTTON_MENU) {
                if (down == EOM_HAL_BUTTON_BACK) {
                    ui_screenshot_save(NULL);
                    ui_toast("%s", _("Screenshot saved."));
                }
            }

            return;
        }
    }

    // Handle Toast Clearing
    if (ui_toast_is_active()) {
        if (ui_toast_is_dismissable() && event == EOM_HAL_BUTTON_PRESS) {
            ui_toast_handle_close();
            ui_toast_clear();
            UI.force_rerender = 1;
        }

        return;
    }

    // Forward to active UI
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
    ui_reset_idle_timer();

    // Toasts always eat encoder...
    if (ui_toast_is_active()) {
        if (ui_toast_scroll(delta) == RENDER) {
            UI.force_rerender = 1;
        }

        return;
    }

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
    UI.last_input_ms = esp_timer_get_time() / 1000UL;
    UI.idle_state = UI_ACTIVE;

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
    ui_input_handle_close(UI.current_input, UI.current_input_arg);

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
        UI.menu_idx_history[UI.menu_history_depth] = ui_menu_get_idx();
        ui_menu_handle_close(current, UI.menu_arg_history[UI.menu_history_depth]);
    }

    UI.menu_history[new_idx] = menu;
    UI.menu_arg_history[new_idx] = arg;
    UI.menu_idx_history[new_idx] = 0;
    UI.menu_history_depth = new_idx;
    UI.force_rerender = RENDER;

    ui_menu_handle_open(menu, arg);
}

void ui_reenter_menu(void) {
    const ui_menu_t* current = UI.menu_history[UI.menu_history_depth];

    if (current != NULL) {
        UI.menu_idx_history[UI.menu_history_depth] = 0;
        ui_menu_handle_close(current, UI.menu_arg_history[UI.menu_history_depth]);
        ui_menu_handle_open(current, UI.menu_arg_history[UI.menu_history_depth]);
    }
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
    UI.menu_idx_history[UI.menu_history_depth] = 0;
    UI.menu_history_depth = new_idx;
    UI.force_rerender = RENDER;

    const ui_menu_t* menu = UI.menu_history[UI.menu_history_depth];

    if (menu != NULL) {
        ui_menu_handle_open(menu, UI.menu_arg_history[UI.menu_history_depth]);
        ui_menu_set_idx(UI.menu_idx_history[UI.menu_history_depth]);
    }
}

void ui_close_all_menu(void) {
    while (UI.menu_history[UI.menu_history_depth] != NULL) {
        ui_close_menu();
    }
}

void ui_close_all(void) {
    ui_close_input();
    ui_close_all_menu();
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

static void render_screensaver_frame() {
    static const int pos_x_max = EOM_DISPLAY_WIDTH - 24;
    static const int pos_y_max = EOM_DISPLAY_HEIGHT - 24;
    static int16_t pos_x = 0;
    static int16_t pos_y = 0;
    static uint8_t direction = 0b11;
    static long last_frame_ms = 0;
    long millis = esp_timer_get_time() / 1000;
    u8g2_t* display = eom_hal_get_display_ptr();

    if (millis - last_frame_ms > (1000 / 20)) {
        last_frame_ms = millis;
        u8g2_ClearBuffer(display);
        pos_x += (direction & 1) ? 1 : -1;
        pos_y += (direction & 2) ? 1 : -1;

        if (pos_x >= pos_x_max || pos_x <= 0) {
            direction ^= 0b01;
        }

        if (pos_y >= pos_y_max || pos_y <= 0) {
            direction ^= 0b10;
        }

        int pressure_icon = orgasm_control_getAveragePressure() / 4095;

        u8g2_SetDrawColor(display, 1);
        u8g2_DrawBitmap(display, pos_x, pos_y, 24 / 8, 24, PLUG_ICON[pressure_icon % 4]);
        ui_send_buffer();
    }
}

void ui_fade_to(uint8_t color) {
    u8g2_t* d = eom_hal_get_display_ptr();
    for (int i = 4; i > 0; i--) {
        ui_draw_pattern_fill(d, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, i);
        ui_send_buffer();
        vTaskDelay(100UL / portTICK_PERIOD_MS);
    }
}

void ui_tick(void) {
    int rendered = 0;
    u8g2_t* display = eom_hal_get_display_ptr();
    uint32_t millis = esp_timer_get_time() / 1000UL;

    // Check idle
    if (UI.idle_state == UI_ACTIVE && Config.screen_dim_seconds != 0 &&
        millis - UI.last_input_ms > ((uint32_t)Config.screen_dim_seconds * 1000UL)) {
        UI.idle_state = UI_IDLE;
        u8g2_SetContrast(display, 0);
    }

    if (UI.idle_state != UI_SCREENSAVER && Config.screen_timeout_seconds != 0 &&
        millis - UI.last_input_ms > ((uint32_t)Config.screen_timeout_seconds * 1000UL)) {
        UI.idle_state = UI_SCREENSAVER;
        u8g2_SetContrast(display, 0);
        ui_fade_to(0);
    }

    if (UI.idle_state == UI_SCREENSAVER) {
        if (Config.enable_screensaver == true) {
            render_screensaver_frame();
        }
        return;
    }

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