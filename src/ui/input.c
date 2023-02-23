#include "ui/input.h"
#include "ui/graphics.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "util/i18n.h"
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
        u8g2_SetFont(d, UI_FONT_SMALL);
        ui_draw_str_center(
            d,
            EOM_DISPLAY_WIDTH / 2,
            value_y + 18,
            i->input.flags.translate_title ? _(i->unit) : i->unit
        );
    }

    ui_draw_button_labels(d, _("BACK"), "", _("SAVE"));
    ui_draw_button_disable(d, 0b010);
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

    default: return;
    }
}

void ui_input_handle_close(const ui_input_t* i, UI_INPUT_ARG_TYPE arg) {
}