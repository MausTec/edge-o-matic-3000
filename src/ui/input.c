#include "ui/input.h"
#include "ui/graphics.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "util/i18n.h"
#include <math.h>
#include <stdio.h>

static int _numeric_value = 0;
static char* _string_buffer = NULL;

static void _set_value(const ui_input_t* i, int save, UI_INPUT_ARG_TYPE arg) {
    switch (i->type) {
    case INPUT_TYPE_NUMERIC: {
        ui_input_numeric_t* input = (ui_input_numeric_t*)i;
        if (save) *(input->value) = _numeric_value;
        if (input->on_save != NULL) input->on_save(_numeric_value, save, arg);
        break;
    }

    case INPUT_TYPE_TOGGLE: {
        ui_input_toggle_t* input = (ui_input_toggle_t*)i;
        if (save) *(input->value) = _numeric_value;
        if (input->on_save != NULL) input->on_save(_numeric_value, save, arg);
        break;
    }

    default: return;
    }
}

ui_render_flag_t ui_input_handle_button(
    const ui_input_t* i,
    eom_hal_button_t button,
    eom_hal_button_event_t event,
    UI_INPUT_ARG_TYPE arg
) {
    if (event == EOM_HAL_BUTTON_PRESS) {
        switch (button) {
        case EOM_HAL_BUTTON_BACK: {
            _set_value(i, 0, arg);
            ui_close_input();
            return RENDER;
        }

        case EOM_HAL_BUTTON_MID: {
            if (i->help != NULL && i->help[0] != '\0') {
                // If help needs to be translated, it will happen here, however, that's a big ask.
                ui_toast_multiline(i->help);
                return RENDER;
            }

            return PASS;
        }

        case EOM_HAL_BUTTON_MENU:
        case EOM_HAL_BUTTON_OK: {
            _set_value(i, 1, arg);
            ui_close_input();
            return RENDER;
        }

        default: return PASS;
        }
    }

    return NORENDER;
}

ui_render_flag_t ui_input_handle_encoder(const ui_input_t* i, int delta, UI_INPUT_ARG_TYPE arg) {
    switch (i->type) {
    case INPUT_TYPE_NUMERIC: {
        int old_value = _numeric_value;
        ui_input_numeric_t* input = (ui_input_numeric_t*)i;

        _numeric_value += delta * input->step;
        if (_numeric_value < input->min)
            _numeric_value = input->min;
        else if (_numeric_value > input->max)
            _numeric_value = input->max;

        if (input->on_save != NULL) input->on_save(_numeric_value, 0, arg);
        return old_value == _numeric_value ? NORENDER : RENDER;
    }

    case INPUT_TYPE_TOGGLE: {
        int old_value = _numeric_value;
        ui_input_toggle_t* input = (ui_input_toggle_t*)i;

        if (delta < 0) {
            _numeric_value = 0;
        } else if (delta > 0) {
            _numeric_value = 1;
        }

        if (input->on_save != NULL) input->on_save(_numeric_value, 0, arg);
        return old_value == _numeric_value ? NORENDER : RENDER;
    }

    default: return PASS;
    }

    return NORENDER;
}

void ui_input_handle_open(const ui_input_t* i, UI_INPUT_ARG_TYPE arg) {
    if (i == NULL) return;

    switch (i->type) {
    case INPUT_TYPE_NUMERIC: {
        ui_input_numeric_t* input = (ui_input_numeric_t*)i;
        _numeric_value = *input->value;
        break;
    }

    case INPUT_TYPE_TOGGLE: {
        ui_input_toggle_t* input = (ui_input_toggle_t*)i;
        _numeric_value = *input->value;
        break;
    }

    default: return;
    }
}

ui_render_flag_t ui_input_handle_loop(const ui_input_t* i, UI_INPUT_ARG_TYPE arg) {
    // i don't really think inputs loop tbh
    return NORENDER;
}

void _render_numeric(const ui_input_numeric_t* i, u8g2_t* d, UI_INPUT_ARG_TYPE arg) {
    char value_str[8];
    sniprintf(value_str, 8, "%d", _numeric_value);

    u8g2_SetFont(d, UI_FONT_LARGE);
    const uint8_t value_y = 24;
    uint8_t width = ui_draw_str_center(d, EOM_DISPLAY_WIDTH / 2, value_y, value_str);

    const uint8_t cursor_offset = 8;
    const uint8_t left_tri_edge = ((EOM_DISPLAY_WIDTH - width) / 2) - cursor_offset;
    const uint8_t right_tri_edge = ((EOM_DISPLAY_WIDTH + width) / 2) + cursor_offset;

    u8g2_DrawTriangle(
        d,
        left_tri_edge,
        value_y + 8,
        left_tri_edge + 3,
        value_y + 5,
        left_tri_edge + 3,
        value_y + 11
    );

    u8g2_DrawTriangle(
        d,
        right_tri_edge,
        value_y + 8,
        right_tri_edge - 3,
        value_y + 5,
        right_tri_edge - 3,
        value_y + 11
    );

    if (i->unit != NULL && i->unit[0] != '\0') {
        if (i->unit[0] == '%') {
            float max_w = EOM_DISPLAY_WIDTH - 24;
            float perc = i->max > 0 ? (float)_numeric_value / i->max : 1.0f;

            if (perc < 0.0f)
                perc = 0.0f;
            else if (perc > 1.0f)
                perc = 1.0f;

            u8g2_DrawHLine(d, 12, value_y + 19 + 2, max_w);
            u8g2_DrawBox(d, 12, value_y + 19, floor(max_w * perc), 5);
        } else {
            u8g2_SetFont(d, UI_FONT_SMALL);
            ui_draw_str_center(
                d,
                EOM_DISPLAY_WIDTH / 2,
                value_y + 18,
                i->input.flags.translate_title ? _(i->unit) : i->unit
            );
        }
    }
}

void _render_toggle(const ui_input_toggle_t* i, u8g2_t* d, UI_INPUT_ARG_TYPE arg) {
    u8g2_SetFont(d, UI_FONT_DEFAULT);
    u8g2_SetFontPosCenter(d);
    const uint8_t value_y = 36;

    ui_draw_str_center(d, (EOM_DISPLAY_WIDTH / 2) - 32, value_y, _("Off"));
    ui_draw_str_center(d, (EOM_DISPLAY_WIDTH / 2) + 32, value_y, _("On"));

    u8g2_DrawRFrame(d, (EOM_DISPLAY_WIDTH / 2) - 8, value_y - 3, 16, 6, 2);

    if (_numeric_value == 0) {
        u8g2_SetDrawColor(d, 0);
        u8g2_DrawRFrame(d, (EOM_DISPLAY_WIDTH / 2) - 11, value_y - 5, 10, 10, 4);
        u8g2_SetDrawColor(d, 1);
        u8g2_DrawRBox(d, (EOM_DISPLAY_WIDTH / 2) - 10, value_y - 4, 8, 8, 3);
    } else {
        u8g2_DrawFrame(d, (EOM_DISPLAY_WIDTH / 2) - 6, value_y - 1, 12, 2);
        u8g2_SetDrawColor(d, 0);
        u8g2_DrawRFrame(d, (EOM_DISPLAY_WIDTH / 2) + 1, value_y - 5, 10, 10, 4);
        u8g2_SetDrawColor(d, 1);
        u8g2_DrawRBox(d, (EOM_DISPLAY_WIDTH / 2) + 2, value_y - 4, 8, 8, 3);
    }
}

void ui_input_handle_render(const ui_input_t* i, u8g2_t* d, UI_INPUT_ARG_TYPE arg) {
    if (i == NULL) return;

    u8g2_SetDrawColor(d, 1);
    ui_toast_draw_frame(d, 2, 4, 10);
    u8g2_SetFont(d, UI_FONT_SMALL);
    u8g2_SetFontPosTop(d);

    // Draw Title
    ui_draw_str_center(
        d, EOM_DISPLAY_WIDTH / 2, 14, i->flags.translate_title ? _(i->title) : i->title
    );

    switch (i->type) {
    case INPUT_TYPE_NUMERIC: {
        ui_input_numeric_t* input = (ui_input_numeric_t*)i;
        _render_numeric(input, d, arg);
        break;
    }

    case INPUT_TYPE_TOGGLE: {
        ui_input_toggle_t* input = (ui_input_toggle_t*)i;
        _render_toggle(input, d, arg);
        break;
    }

    default: return;
    }

    if (i->help != NULL && i->help[0] != '\0') {
        ui_draw_button_labels(d, _("BACK"), _("HELP"), _("SAVE"));
    } else {
        ui_draw_button_labels(d, _("BACK"), "", _("SAVE"));
        ui_draw_button_disable(d, 0b010);
    }
}

void ui_input_handle_close(const ui_input_t* i, UI_INPUT_ARG_TYPE arg) {
}