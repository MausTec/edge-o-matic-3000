#include "esp_timer.h"
#include "orgasm_control.h"
#include "ui/ui.h"
#include "util/i18n.h"

static struct {
    uint16_t arousal_peak;
    uint16_t arousal_peak_last_ms;
    uint16_t arousal_peak_update_ms;
} state;

static void on_open(void* arg) {
}

static ui_render_flag_t on_loop(void* arg) {
    ui_render_flag_t render = NORENDER;
    uint32_t millis = esp_timer_get_time() / 1000UL;

    if (orgasm_control_updated()) {
        orgasm_control_clear_update_flag();

        // Update Arousal Peak
        uint16_t arousal = orgasm_control_getArousal();
        if (arousal > state.arousal_peak) {
            state.arousal_peak = arousal;
            state.arousal_peak_last_ms = millis;
        }

        render = RENDER;
    }

    if (state.arousal_peak > 0 && millis - state.arousal_peak_last_ms > 3000 &&
        millis - state.arousal_peak_update_ms > 100) {
        state.arousal_peak *= 0.95;
        state.arousal_peak_update_ms = millis;
        render = RENDER;
    }

    return render;
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

    // Motor and Arousal bars
    if (orgasm_control_get_output_mode() == OC_MANUAL_CONTROL)
        ui_draw_bar_graph(d, 10, 'M', eom_hal_get_motor_speed(), 255);
    else
        ui_draw_shaded_bar_graph(
            d, 10, 'M', eom_hal_get_motor_speed(), 255, Config.motor_max_speed
        );

    ui_draw_shaded_bar_graph_with_peak(
        d,
        EOM_DISPLAY_HEIGHT - 19,
        'A',
        orgasm_control_getArousal(),
        Config.sensitivity_threshold * 1.5,
        Config.sensitivity_threshold,
        state.arousal_peak
    );

    // Pressure Icon

    // Pressure and Sensitivity Meter

    // Denial Limit
}

static void on_close(void* arg) {
}

static ui_render_flag_t
on_button(eom_hal_button_t button, eom_hal_button_event_t event, void* arg) {
    oc_bool_t locked = orgasm_control_isMenuLocked();

    if (event != EOM_HAL_BUTTON_PRESS)
        return PASS;

    if (button == EOM_HAL_BUTTON_BACK) {
        // ui open chart
    } else if (button == EOM_HAL_BUTTON_MID) {
        if (locked)
            return NORENDER;

        orgasm_control_controlMotor(OC_MANUAL_CONTROL);
        eom_hal_set_motor_speed(0x00);
    } else if (button == EOM_HAL_BUTTON_OK) {
        if (locked)
            return NORENDER;

        orgasm_output_mode_t mode = orgasm_control_get_output_mode();

        if (mode == OC_MANUAL_CONTROL) {
            orgasm_control_controlMotor(OC_AUTOMAITC_CONTROL);
        } else if (mode == OC_AUTOMAITC_CONTROL) {
            orgasm_control_controlMotor(OC_LOCKOUT_POST_MODE);
        } else {
            orgasm_control_controlMotor(OC_MANUAL_CONTROL);
        }
    } else {
        return PASS;
    }

    return RENDER;
}

const struct ui_page PAGE_EDGING_STATS = {
    .title = "Edging Stats",
    .on_open = on_open,
    .on_render = on_render,
    .on_loop = on_loop,
    .on_button = on_button,
};