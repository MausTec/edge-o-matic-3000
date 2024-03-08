#include "accessory_driver.h"
#include "assets.h"
#include "bluetooth_driver.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "menus/index.h"
#include "orgasm_control.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "util/i18n.h"
#include <math.h>
#include <string.h>

#define CHANGE_NOTICE_DELAY_MS 1000UL

static const char* TAG = "page:edging_stats";

static struct {
    uint16_t arousal_peak;
    uint64_t arousal_peak_last_ms;
    uint64_t arousal_peak_update_ms;
    uint64_t speed_change_notice_ms;
    uint64_t arousal_change_notice_ms;
} state;

static void on_open(void* arg) {
    // ui_toast_multiline(
    //     "Richter: Die monster. You don't belong in this world!\n\n"
    //     "Dracula: It was not by my hand that I am once again given flesh. I was called here by "
    //     "humans who wish to pay me tribute.\n\n"
    //     "Richter: Tribute!?! You steal men's souls and make them your slaves!\n\n"
    //     "Dracula: Perhaps the same could be said of all religions...\n\n"
    //     "Richter: Your words are as empty as your soul! Mankind ill needs a savior such as
    //     you!\n\n" "Dracula: What is a man? (throws his wine glass to the floor) A miserable
    //     little pile of " "secrets. But enough talk... Have at you!\n\n" "Belmont:
    //     alsdjf;lskdfj;alskdjf;laskdjf;alskdjf;alksdjflsdkf <3"
    // );
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
        state.arousal_peak *= 0.90;
        state.arousal_peak_update_ms = millis;
        render = RENDER;
    }

    // Clear notices
    if (state.speed_change_notice_ms > 0 && millis > state.speed_change_notice_ms) {
        state.speed_change_notice_ms = 0;
        render = RENDER;
    }

    if (state.arousal_change_notice_ms > 0 && millis > state.arousal_change_notice_ms) {
        state.arousal_change_notice_ms = 0;
        render = RENDER;
    }

    return render;
}

static void _draw_buttons(u8g2_t* d, orgasm_output_mode_t mode) {
    const char* btn1 = _("MENU");
    const char* btn2 = _("STOP");

    if (orgasm_control_isMenuLocked()) {
        ui_draw_button_labels(d, btn1, _("LOCKED"), _("LOCKED"));
        ui_draw_button_disable(d, 0b011);
    } else if (mode == OC_MANUAL_CONTROL) {
        ui_draw_button_labels(d, btn1, btn2, _("AUTO"));
    } else if (mode == OC_AUTOMAITC_CONTROL) {
        if (Config.use_orgasm_modes == true) {
            ui_draw_button_labels(d, btn1, btn2, _("POST"));
        } else {
            ui_draw_button_labels(d, btn1, btn2, _("MANUAL"));
        }
    } else if (mode == OC_ORGASM_MODE) {
        ui_draw_button_labels(d, btn1, btn2, _("MANUAL"));
    }
}

static void _draw_status(u8g2_t* d, orgasm_output_mode_t mode) {
    if (mode == OC_AUTOMAITC_CONTROL) {
        ui_draw_status(d, _("Auto Edging"));
    } else if (mode == OC_MANUAL_CONTROL) {
        ui_draw_status(d, _("Manual"));
    } else if (mode == OC_ORGASM_MODE) {
        ui_draw_status(d, _("Edging+Orgasm"));
    } else {
        ui_draw_status(d, "---");
    }
}

float _get_arousal_bar_max(void) {
    static int last_sens_thresh = 0;
    static float last_arousal_max = 0;

    // Memoize to prevent recalculating on renders.
    if (Config.sensitivity_threshold != last_sens_thresh) {
        last_sens_thresh = Config.sensitivity_threshold;
        last_arousal_max =
            pow10f(ceilf(log10f(Config.sensitivity_threshold * 1.10f) * 4.0f) / 4.0f);
    }

    return last_arousal_max;
}

static void _draw_meters(u8g2_t* d, orgasm_output_mode_t mode) {
    if (mode == OC_MANUAL_CONTROL) {
        ui_draw_bar_graph(d, 10, 'S', eom_hal_get_motor_speed(), 255);
    } else {
        ui_draw_shaded_bar_graph(
            d, 10, 'S', eom_hal_get_motor_speed(), 255, Config.motor_max_speed
        );
    }

    // Arousal Bar
    ui_draw_shaded_bar_graph_with_peak(
        d,
        EOM_DISPLAY_HEIGHT - 18,
        'A',
        orgasm_control_getArousal(),
        _get_arousal_bar_max(),
        Config.sensitivity_threshold,
        state.arousal_peak
    );
}

static void _draw_speed_change(u8g2_t* d) {
    char msg[15];
    snprintf(msg, sizeof(msg), _("Speed: %0.0f%%"), (float)eom_hal_get_motor_speed() / 2.55f);

    u8g2_SetFont(d, UI_FONT_DEFAULT);
    u8g2_SetDrawColor(d, 1);
    u8g2_SetFontPosCenter(d);

    ui_draw_str_center(d, EOM_DISPLAY_WIDTH / 2, EOM_DISPLAY_HEIGHT / 2, msg);
}

static void _draw_arousal_change(u8g2_t* d) {
    char msg[16];
    snprintf(msg, sizeof(msg), _("Threshold: %d"), orgasm_control_get_arousal_threshold());

    u8g2_SetFont(d, UI_FONT_DEFAULT);
    u8g2_SetDrawColor(d, 1);
    u8g2_SetFontPosCenter(d);

    ui_draw_str_center(d, EOM_DISPLAY_WIDTH / 2, EOM_DISPLAY_HEIGHT / 2, msg);
}

static void _draw_pressure_icon(u8g2_t* d) {
    const uint16_t PRESSURE_MAX = 4095;
    const uint8_t MID_HEIGHT = 20;
    const uint8_t BAR_LEFT = 26;

    uint16_t pressure = orgasm_control_getLastPressure();
    uint8_t pressure_idx = pressure / (PRESSURE_MAX / 4);
    float pressure_pct = (float)pressure / PRESSURE_MAX;
    float sensitivity_pct = (float)Config.sensor_sensitivity / 255.0f;

    char pres_str[4];
    char sens_str[4];
    char pres_str_full[16];
    char sens_str_full[16];

    if (pressure_pct >= 1.0f) {
        strncpy(pres_str, _("MAX"), 4);
    } else {
        snprintf(pres_str, 4, "%02d%%", (int)floor(pressure_pct * 100.0f));
    }

    if (sensitivity_pct >= 1.0f) {
        strncpy(sens_str, _("MAX"), 4);
    } else {
        snprintf(sens_str, 4, "%02d%%", (int)floor(sensitivity_pct * 100.0f));
    }

    snprintf(pres_str_full, 16, _("Pres: %s"), pres_str);
    snprintf(sens_str_full, 16, _("Sens: %s"), sens_str);

    u8g2_SetDrawColor(d, 1);
    u8g2_SetFont(d, UI_FONT_DEFAULT);
    u8g2_SetFontPosTop(d);
    u8g2_DrawBitmap(d, 0, 20, 24 / 8, 24, PLUG_ICON[pressure_idx]);
    u8g2_DrawUTF8(d, BAR_LEFT, MID_HEIGHT, pres_str_full);
    u8g2_DrawUTF8(d, BAR_LEFT, MID_HEIGHT + 16, sens_str_full);

    // Draw Bar
    uint8_t line_width = u8g2_GetUTF8Width(d, pres_str_full);
    uint8_t pres_width = (int)floor(pressure_pct * (line_width - 7));

    if (pres_width > 0) {
        u8g2_DrawHLine(d, BAR_LEFT, MID_HEIGHT + 12, 1 + pres_width);
    }

    if (pres_width < line_width - 7) {
        u8g2_DrawHLine(d, BAR_LEFT + pres_width + 6, MID_HEIGHT + 12, line_width - pres_width - 7);
    }

    u8g2_DrawVLine(d, BAR_LEFT, MID_HEIGHT + 10, 5);
    u8g2_DrawVLine(d, BAR_LEFT + line_width - 1, MID_HEIGHT + 10, 5);

    u8g2_DrawHLine(d, BAR_LEFT + pres_width + 1, MID_HEIGHT + 10, 5);
    u8g2_DrawHLine(d, BAR_LEFT + pres_width + 2, MID_HEIGHT + 11, 3);
    u8g2_DrawPixel(d, BAR_LEFT + pres_width + 3, MID_HEIGHT + 12);
}

static void _draw_denial_count(u8g2_t* d) {
    int denial = orgasm_control_getDenialCount();
    const uint8_t MID_HEIGHT = 20;
    const uint8_t DENIAL_WIDTH = 41;
    char denial_str[4];

    snprintf(denial_str, 4, "%03d", denial);

    u8g2_SetDrawColor(d, 1);
    u8g2_DrawVLine(
        d,
        EOM_DISPLAY_WIDTH - DENIAL_WIDTH - 2,
        MID_HEIGHT + 1,
        EOM_DISPLAY_HEIGHT - MID_HEIGHT - 21
    );

    u8g2_SetFont(d, UI_FONT_SMALL);
    u8g2_SetFontPosTop(d);

    ui_draw_str_center(d, EOM_DISPLAY_WIDTH - (DENIAL_WIDTH / 2), MID_HEIGHT, _("Denied"));

    u8g2_SetFont(d, UI_FONT_LARGE);
    ui_draw_str_center(d, EOM_DISPLAY_WIDTH - (DENIAL_WIDTH / 2), MID_HEIGHT + 9, denial_str);
}

static void on_render(u8g2_t* d, void* arg) {
    orgasm_output_mode_t mode = orgasm_control_get_output_mode();

    _draw_buttons(d, mode);
    _draw_status(d, mode);
    _draw_meters(d, mode);

    if (state.speed_change_notice_ms > 0) {
        _draw_speed_change(d);
    } else if (state.arousal_change_notice_ms > 0) {
        _draw_arousal_change(d);
    } else {
        _draw_pressure_icon(d);
        _draw_denial_count(d);
    }
}

static void on_close(void* arg) {
}

static ui_render_flag_t
on_button(eom_hal_button_t button, eom_hal_button_event_t event, void* arg) {
    oc_bool_t locked = orgasm_control_isMenuLocked();

    if (event != EOM_HAL_BUTTON_PRESS) return PASS;

    if (button == EOM_HAL_BUTTON_BACK) {
        ui_open_menu(&EDGING_MODE_MENU, NULL);
    } else if (button == EOM_HAL_BUTTON_MID) {
        if (locked) return NORENDER;

        orgasm_control_set_output_mode(OC_MANUAL_CONTROL);
        eom_hal_set_motor_speed(0x00);
        accessory_driver_broadcast_speed(0x00);
        bluetooth_driver_broadcast_speed(0x00);
        // websocket_server_broadcast_speed(0x00);
    } else if (button == EOM_HAL_BUTTON_OK) {
        if (locked) return NORENDER;

        orgasm_output_mode_t mode = orgasm_control_get_output_mode();

        if (mode == OC_MANUAL_CONTROL) {
            orgasm_control_set_output_mode(OC_AUTOMAITC_CONTROL);
        } else if (mode == OC_AUTOMAITC_CONTROL) {
            if (Config.use_orgasm_modes == true) {
                orgasm_control_set_output_mode(OC_ORGASM_MODE);
            } else {
                orgasm_control_set_output_mode(OC_MANUAL_CONTROL);
            }
        } else {
            orgasm_control_set_output_mode(OC_MANUAL_CONTROL);
        }
    } else {
        return PASS;
    }

    return RENDER;
}

static ui_render_flag_t on_encoder(int delta, void* arg) {
    oc_bool_t locked = orgasm_control_isMenuLocked();
    orgasm_output_mode_t mode = orgasm_control_get_output_mode();
    uint64_t millis = esp_timer_get_time() / 1000UL;

    if (mode == OC_MANUAL_CONTROL) {
        int speed = eom_hal_get_motor_speed() + delta;
        uint8_t speed_byte = speed > 0xFF ? 0xFF : (speed < 0x00 ? 0x00 : speed);
        eom_hal_set_motor_speed(speed_byte);
        accessory_driver_broadcast_speed(speed_byte);
        bluetooth_driver_broadcast_speed(speed_byte);
        // websocket_server_broadcast_speed(speed_byte);
        state.arousal_change_notice_ms = 0;
        state.speed_change_notice_ms = millis + CHANGE_NOTICE_DELAY_MS;
        return RENDER;
    }

    if (mode != OC_MANUAL_CONTROL) {
        orgasm_control_increment_arousal_threshold(delta);
        state.speed_change_notice_ms = 0;
        state.arousal_change_notice_ms = millis + CHANGE_NOTICE_DELAY_MS;
        return RENDER;
    }

    return NORENDER;
}

const struct ui_page PAGE_EDGING_STATS = {
    .title = "Edging Stats",
    .on_open = on_open,
    .on_render = on_render,
    .on_loop = on_loop,
    .on_button = on_button,
    .on_encoder = on_encoder,
};