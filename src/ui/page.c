#include "ui/page.h"
#include "esp_log.h"
#include "ui/ui.h"

static const char* TAG = "ui:page";

ui_render_flag_t ui_page_handle_button(
    const ui_page_t* p, eom_hal_button_t button, eom_hal_button_event_t event, void* arg
) {
    ui_render_flag_t rf = PASS;

    if (p != NULL && p->on_button != NULL) {
        rf = p->on_button(button, event, arg);
    }

    // default action
    if (rf == PASS) {
        if (button == EOM_HAL_BUTTON_MENU && event == EOM_HAL_BUTTON_PRESS) {
            ui_open_menu(&MAIN_MENU, NULL);
        }
    }

    return rf;
}

ui_render_flag_t ui_page_handle_encoder(const ui_page_t* p, int delta, void* arg) {
    ui_render_flag_t rf = PASS;

    if (p != NULL && p->on_encoder != NULL) {
        rf = p->on_encoder(delta, arg);
    }

    if (rf == PASS) {
        // default action
    }

    return rf;
}

void ui_page_handle_open(const ui_page_t* p, void* arg) {
    if (p != NULL && p->on_open != NULL) {
        p->on_open(arg);
    }
}

ui_render_flag_t ui_page_handle_loop(const ui_page_t* p, void* arg) {
    ui_render_flag_t rf = PASS;

    if (p != NULL && p->on_loop != NULL) {
        rf = p->on_loop(arg);
    }

    if (rf == PASS) {
        // default action
    }

    return rf;
}

void ui_page_handle_render(const ui_page_t* p, u8g2_t* d, void* arg) {
    if (p != NULL && p->on_render != NULL) {
        p->on_render(d, arg);
    }
}

void ui_page_handle_close(const ui_page_t* p, void* arg) {
    if (p != NULL && p->on_close != NULL) {
        p->on_close(arg);
    }
}
