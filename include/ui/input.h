#ifndef __ui__input_h
#define __ui__input_h

#ifdef __cplusplus
extern "C" {
#endif

#include "eom-hal.h"
#include "ui/render.h"
#include <stdbool.h>
#include <stdint.h>

#define UI_INPUT_ARG_TYPE const void*

typedef struct ui_input_render_flags {
    uint8_t translate_title : 1;
} ui_input_render_flags_t;

typedef enum ui_input_type {
    INPUT_TYPE_NUMERIC,
    INPUT_TYPE_BYTE,
    INPUT_TYPE_TOGGLE,
    INPUT_TYPE_SELECT,
    INPUT_TYPE_MULTISELECT,
    INPUT_TYPE_TEXT
} ui_input_type_t;

typedef struct ui_input {
    const char* title;
    const char* help;
    ui_input_render_flags_t flags;
    ui_input_type_t type;
} ui_input_t;

// Numeric Input

typedef void (*ui_input_numeric_save_cb)(int value, int final, UI_INPUT_ARG_TYPE arg);
typedef int (*ui_input_numeric_chart_getter_cb)(int value, int final, UI_INPUT_ARG_TYPE arg);

typedef struct ui_input_numeric {
    ui_input_t input;
    const char* unit;
    int* value;
    int min;
    int max;
    int step;
    ui_input_numeric_save_cb on_save;
    ui_input_numeric_chart_getter_cb chart_getter;
} ui_input_numeric_t;

#define IntegerInput(titlestr, pValue, unitstr, saveCb)                                            \
    {                                                                                              \
        .input = { .title = titlestr,                                                              \
                   .flags = { .translate_title = 1 },                                              \
                   .help = NULL,                                                                   \
                   .type = INPUT_TYPE_NUMERIC },                                                   \
        .unit = unitstr, .value = pValue, .min = INT_MIN, .max = INT_MAX, .step = 1,               \
        .on_save = saveCb, .chart_getter = NULL                                                    \
    }

#define UnsignedInputValues(titlestr, pValue, unitstr, saveCb)                                     \
    .input = { .title = titlestr,                                                                  \
               .flags = { .translate_title = 1 },                                                  \
               .help = NULL,                                                                       \
               .type = INPUT_TYPE_NUMERIC },                                                       \
    .unit = unitstr, .value = pValue, .min = 0, .max = INT_MAX, .step = 1, .on_save = saveCb,      \
    .chart_getter = NULL

#define UnsignedInput(titlestr, pValue, unitstr, saveCb)                                           \
    { UnsignedInputValues(titlestr, pValue, unitstr, saveCb) }

typedef struct ui_input_byte {
    ui_input_t input;
    const char* unit;
    uint8_t* value;
    uint8_t min;
    uint8_t max;
    uint8_t step;
    ui_input_numeric_save_cb on_save;
    ui_input_numeric_chart_getter_cb chart_getter;
} ui_input_byte_t;

#define ByteInputValues(titlestr, pValue, unitstr, saveCb)                                         \
    .input = { .title = titlestr,                                                                  \
               .flags = { .translate_title = 1 },                                                  \
               .help = NULL,                                                                       \
               .type = INPUT_TYPE_BYTE },                                                          \
    .unit = unitstr, .value = pValue, .min = 0, .max = 0xFF, .step = 1, .on_save = saveCb,         \
    .chart_getter = NULL

#define ByteInput(titlestr, pValue, unitstr, saveCb)                                               \
    { ByteInputValues(titlestr, pValue, unitstr, saveCb) }

// Toggle Input

typedef struct ui_input_toggle {
    ui_input_t input;
    bool* value;
    ui_input_numeric_save_cb on_save;
} ui_input_toggle_t;

#define ToggleInputValues(titlestr, pValue, saveCb)                                                \
    .input = { .title = titlestr,                                                                  \
               .flags = { .translate_title = 1 },                                                  \
               .help = NULL,                                                                       \
               .type = INPUT_TYPE_TOGGLE },                                                        \
    .value = pValue, .on_save = saveCb

#define ToggleInput(titlestr, pValue, saveCb)                                                      \
    { ToggleInputValues(titlestr, pValue, saveCb) }

// Select Inputs

typedef struct ui_input_select_option {
    const char* label;
    int ival;
    void* value;
} ui_input_select_option_t;

typedef void (*ui_input_select_save_cb
)(const ui_input_select_option_t* value, int final, UI_INPUT_ARG_TYPE arg);

typedef struct ui_input_select {
    ui_input_t input;
    ui_input_select_option_t (*options)[];
    size_t option_count;
    int* value;
    ui_input_select_save_cb on_save;
} ui_input_select_t;

typedef struct ui_input_multiselect {
    ui_input_select_t select;
} ui_input_multiselect_t;

#define SelectInputValues(titlestr, pValue, pOptions, nOptions, saveCb)                            \
    .input = { .title = titlestr,                                                                  \
               .flags = { .translate_title = 1 },                                                  \
               .help = NULL,                                                                       \
               .type = INPUT_TYPE_SELECT },                                                        \
    .options = pOptions, .option_count = nOptions, .on_save = saveCb, .value = pValue

#define SelectInput(titlestr, pValue, pOptions, nOptions, saveCb)                                  \
    { SelectInputValues(titlestr, pValue, pOptions, nOptions, saveCb) }

// Text Input

typedef struct ui_input_text {
    ui_input_t input;
} ui_input_text_t;

// Lifecycle Methods

ui_render_flag_t ui_input_handle_button(
    const ui_input_t* i,
    eom_hal_button_t button,
    eom_hal_button_event_t event,
    UI_INPUT_ARG_TYPE arg
);

ui_render_flag_t ui_input_handle_encoder(const ui_input_t* i, int delta, UI_INPUT_ARG_TYPE arg);
void ui_input_handle_open(const ui_input_t* i, UI_INPUT_ARG_TYPE arg);
ui_render_flag_t ui_input_handle_loop(const ui_input_t* i, UI_INPUT_ARG_TYPE arg);
void ui_input_handle_render(const ui_input_t* i, u8g2_t* d, UI_INPUT_ARG_TYPE arg);
void ui_input_handle_close(const ui_input_t* i, UI_INPUT_ARG_TYPE arg);

#ifdef __cplusplus
}
#endif

#endif
