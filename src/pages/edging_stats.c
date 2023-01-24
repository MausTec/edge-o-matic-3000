#include "orgasm_control.h"
#include "ui/ui.h"
#include "util/i18n.h"

static void on_open(void* arg) {
}

static ui_render_flag_t on_loop(void* arg) {
    return NORENDER;
}

static void _draw_buttons(u8g2_t* d, orgasm_output_mode_t mode) {
    const char* btn1 = _("CHART");
    const char* btn2 = _("STOP");

    if (orgasm_control_isMenuLocked()) {
        ui_draw_button_labels(d, btn1, _("LOCKED"), _("LOCKED"));
        ui_draw_button_disable(d, 0b011);
    } else if (mode == OC_AUTOMAITC_CONTROL) {
        ui_draw_button_labels(d, btn1, btn2, _("POST"));
    } else if (mode == OC_MANUAL_CONTROL) {
        ui_draw_button_labels(d, btn1, btn2, _("AUTO"));
    } else if (mode == OC_LOCKOUT_POST_MODE) {
        ui_draw_button_labels(d, btn1, btn2, _("MANUAL"));
    }
}

static void _draw_status(u8g2_t* d, orgasm_output_mode_t mode) {
    if (mode == OC_AUTOMAITC_CONTROL)
        ui_draw_status(d, _("Auto Edging"));
    if (mode == OC_MANUAL_CONTROL)
        ui_draw_status(d, _("Manual"));
    if (mode == OC_LOCKOUT_POST_MODE)
        ui_draw_status(d, _("Edging+Orgasm"));
    else
        ui_draw_status(d, "---");
}

static void on_render(u8g2_t* d, void* arg) {
    orgasm_output_mode_t mode = orgasm_control_get_output_mode();

    _draw_buttons(d, mode);
    _draw_status(d, mode);
}

static void on_exit(void* arg) {
}

const struct ui_page PAGE_EDGING_STATS = {
    .title = "Edging Stats",
    .on_open = on_open,
    .on_render = on_render,
    .on_loop = on_loop,
};