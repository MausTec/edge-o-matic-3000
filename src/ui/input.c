#include "ui/input.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "ui/graphics.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "util/i18n.h"
#include <math.h>
#include <stdio.h>

static const char* TAG = "ui:input";

static int _numeric_value = 0;
static int _chart_value = 0;
static char* _string_buffer = NULL;

static void _set_value(const ui_input_t* i, int save, UI_INPUT_ARG_TYPE arg) {
    switch (i->type) {
    case INPUT_TYPE_NUMERIC: {
        ui_input_numeric_t* input = (ui_input_numeric_t*)i;
        if (save && input->value != NULL) *(input->value) = _numeric_value;
        if (input->on_save != NULL) input->on_save(_numeric_value, save, arg);
        if (input->chart_getter != NULL) {
            _chart_value = input->chart_getter(_numeric_value, save, arg);
        } else {
            _chart_value = _numeric_value;
        }
        break;
    }

    case INPUT_TYPE_BYTE: {
        ui_input_byte_t* input = (ui_input_byte_t*)i;

        if (save && input->value != NULL) *(input->value) = (uint8_t)_numeric_value;
        if (input->on_save != NULL) input->on_save(_numeric_value, save, arg);
        if (input->chart_getter != NULL) {
            _chart_value = input->chart_getter(_numeric_value, save, arg);
        } else {
            _chart_value = _numeric_value;
        }
        break;
    }

    case INPUT_TYPE_TOGGLE: {
        ui_input_toggle_t* input = (ui_input_toggle_t*)i;
        if (save && input->value != NULL) *(input->value) = (uint8_t)_numeric_value;
        if (input->on_save != NULL) input->on_save(_numeric_value, save, arg);
        break;
    }

    case INPUT_TYPE_SELECT: {
        ui_input_select_t* input = (ui_input_select_t*)i;
        ui_input_select_option_t* option = &(*input->options)[_numeric_value];
        if (save && input->value != NULL) *(input->value) = option->ival;
        if (input->on_save != NULL) input->on_save(option, save, arg);
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
    if (event == EOM_HAL_BUTTON_HOLD) {
        switch (button) {
        case EOM_HAL_BUTTON_BACK: {
            ui_close_all();
            return RENDER;
        }

        case EOM_HAL_BUTTON_MID: {
            // resture default
            ui_input_handle_close(i, arg);
            return RENDER;
        }

        default: return NORENDER;
        }
    }

    else if (event == EOM_HAL_BUTTON_PRESS) {
        switch (button) {
        case EOM_HAL_BUTTON_BACK: {
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

        _set_value(i, 0, arg);
        return old_value == _numeric_value ? NORENDER : RENDER;
    }

    case INPUT_TYPE_BYTE: {
        int old_value = _numeric_value;
        ui_input_byte_t* input = (ui_input_byte_t*)i;

        _numeric_value += delta * input->step;
        if (_numeric_value < input->min)
            _numeric_value = input->min;
        else if (_numeric_value > input->max)
            _numeric_value = input->max;

        _set_value(i, 0, arg);
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

        _set_value(i, 0, arg);
        return old_value == _numeric_value ? NORENDER : RENDER;
    }

    case INPUT_TYPE_SELECT: {
        int old_value = _numeric_value;
        ui_input_select_t* input = (ui_input_select_t*)i;

        if (delta < 0) {
            _numeric_value = (_numeric_value == 0) ? input->option_count - 1 : _numeric_value - 1;
        } else if (delta > 0) {
            _numeric_value = (_numeric_value >= input->option_count - 1) ? 0 : _numeric_value + 1;
        }

        _set_value(i, 0, arg);
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

    case INPUT_TYPE_BYTE: {
        ui_input_byte_t* input = (ui_input_byte_t*)i;
        _numeric_value = (int)*input->value;
        break;
    }

    case INPUT_TYPE_TOGGLE: {
        ui_input_toggle_t* input = (ui_input_toggle_t*)i;
        _numeric_value = (int)*input->value;
        break;
    }

    case INPUT_TYPE_SELECT: {
        ui_input_select_t* input = (ui_input_select_t*)i;
        _numeric_value = 0;

        if (input->value != NULL) {
            for (int idx = 0; idx < input->option_count; idx++) {
                if ((*input->options)[idx].ival == *input->value) {
                    _numeric_value = idx;
                    break;
                }
            }
        }

        break;
    }

    default: break;
    }

    _set_value(i, 0, arg);
}

ui_render_flag_t ui_input_handle_loop(const ui_input_t* i, UI_INPUT_ARG_TYPE arg) {
    static unsigned long last_update_ms = 0;
    unsigned long millis = esp_timer_get_time() / 1000UL;

    if (i->type == INPUT_TYPE_NUMERIC && millis - last_update_ms > 100) {
        last_update_ms = millis;
        ui_input_numeric_t* input = (ui_input_numeric_t*)i;

        if (input->chart_getter != NULL) {
            int old_value = _chart_value;
            _chart_value = input->chart_getter(_numeric_value, 0, arg);
            return old_value == _chart_value ? NORENDER : RENDER;
        }
    }

    return NORENDER;
}

void _render_numeric(const ui_input_t* i, u8g2_t* d, UI_INPUT_ARG_TYPE arg) {
    const char* unit = NULL;
    int min = INT_MIN;
    int max = INT_MAX;

    if (i->type == INPUT_TYPE_NUMERIC) {
        ui_input_numeric_t* input = (ui_input_numeric_t*)i;
        unit = input->unit;
        min = input->min;
        max = input->max;
    } else if (i->type == INPUT_TYPE_BYTE) {
        ui_input_byte_t* input = (ui_input_byte_t*)i;
        unit = input->unit;
        min = input->min;
        max = input->max;
    }

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

    if (unit != NULL && unit[0] != '\0') {
        if (unit[0] == '%') {
            float max_w = EOM_DISPLAY_WIDTH - 24;
            float perc = (max - min) > 0 ? (float)(_chart_value - min) / (max - min) : 1.0f;

            if (perc < 0.0f)
                perc = 0.0f;
            else if (perc > 1.0f)
                perc = 1.0f;

            u8g2_DrawHLine(d, 12, value_y + 19 + 2, max_w);
            u8g2_DrawBox(d, 12, value_y + 19, floor(max_w * perc), 5);
        } else {
            u8g2_SetFont(d, UI_FONT_SMALL);
            ui_draw_str_center(
                d, EOM_DISPLAY_WIDTH / 2, value_y + 18, i->flags.translate_title ? _(unit) : unit
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

void _render_select(const ui_input_select_t* i, u8g2_t* d, UI_INPUT_ARG_TYPE arg) {
    u8g2_SetFontPosCenter(d);
    const uint8_t value_y = 34;
    const uint8_t value_c = (EOM_DISPLAY_WIDTH / 2) - 4;

    u8g2_DrawBox(d, 12, value_y - 6, EOM_DISPLAY_WIDTH - 24, 11);

    const ui_input_select_option_t* prev = NULL;
    const ui_input_select_option_t* next = NULL;
    const ui_input_select_option_t* current = NULL;

    if (_numeric_value < 0 || _numeric_value > i->option_count - 1) {
        ESP_LOGW(TAG, "_numeric_value out of bounds: %d", _numeric_value);
        _numeric_value = 0;
    }

    if (_numeric_value > 0) {
        prev = &(*i->options)[_numeric_value - 1];
    } else {
        prev = &(*i->options)[i->option_count - 1];
    }

    current = &(*i->options)[_numeric_value];

    if (_numeric_value < i->option_count - 1) {
        next = &(*i->options)[_numeric_value + 1];
    } else {
        next = &(*i->options)[0];
    }

    u8g2_SetFont(d, UI_FONT_SMALL);

    if (prev != NULL && prev->label != NULL) {
        ui_draw_str_center(d, value_c, value_y - 10, prev->label);
    }

    if (next != NULL && next->label != NULL) {
        ui_draw_str_center(d, value_c, value_y + 11, next->label);
    }

    if (current != NULL && current->label != NULL) {
        u8g2_SetDrawColor(d, 0);
        u8g2_SetFont(d, UI_FONT_DEFAULT);
        ui_draw_str_center(d, value_c, value_y, current->label);
    }
}

void ui_input_handle_render(const ui_input_t* i, u8g2_t* d, UI_INPUT_ARG_TYPE arg) {
    if (i == NULL) return;

    u8g2_SetDrawColor(d, 1);
    ui_toast_draw_frame(d, 2, 4, 5, EOM_DISPLAY_HEIGHT - 5 - 9);
    u8g2_SetFont(d, UI_FONT_SMALL);
    u8g2_SetFontPosTop(d);

    u8g2_DrawBox(d, 7, 8, EOM_DISPLAY_WIDTH - 14, 7);
    u8g2_SetDrawColor(d, 0);

    // Draw Title
    ui_draw_str_center(
        d, EOM_DISPLAY_WIDTH / 2, 7, i->flags.translate_title ? _(i->title) : i->title
    );

    u8g2_SetDrawColor(d, 1);

    switch (i->type) {
    case INPUT_TYPE_BYTE:
    case INPUT_TYPE_NUMERIC: {
        _render_numeric(i, d, arg);
        break;
    }

    case INPUT_TYPE_TOGGLE: {
        ui_input_toggle_t* input = (ui_input_toggle_t*)i;
        _render_toggle(input, d, arg);
        break;
    }

    case INPUT_TYPE_SELECT: {
        ui_input_select_t* input = (ui_input_select_t*)i;
        _render_select(input, d, arg);
        break;
    }

    default: break; ;
    }

    if (i->help != NULL && i->help[0] != '\0') {
        ui_draw_button_labels(d, _("BACK"), _("HELP"), _("SAVE"));
    } else {
        ui_draw_button_labels(d, _("BACK"), "", _("SAVE"));
        ui_draw_button_disable(d, 0b010);
    }
}

void ui_input_handle_close(const ui_input_t* i, UI_INPUT_ARG_TYPE arg) {
    if (i == NULL) return;

    switch (i->type) {
    case INPUT_TYPE_NUMERIC: {
        ui_input_numeric_t* input = (ui_input_numeric_t*)i;
        _numeric_value = *input->value;
        _set_value(i, 0, arg);
        break;
    }

    case INPUT_TYPE_TOGGLE: {
        ui_input_toggle_t* input = (ui_input_toggle_t*)i;
        _numeric_value = *input->value;
        _set_value(i, 0, arg);
        break;
    }

    case INPUT_TYPE_SELECT: {
        // Select inputs don't really do anything until you save, so...

        break;
    }

    default: break; ;
    }
}