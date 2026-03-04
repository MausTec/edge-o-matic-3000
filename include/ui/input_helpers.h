#ifndef __ui__input_helpers_h
#define __ui__input_helpers_h

#ifdef __cplusplus
extern "C" {
#endif

#include "ui/input.h"

/**
 * @brief Union type for dynamic input allocation/recycling.
 * Leverages the fact that all input types embed ui_input_t as first member.
 * This allows setting common base fields once, then accessing type-specific fields.
 */
typedef union ui_input_any {
    ui_input_t base;
    ui_input_toggle_t toggle;
    ui_input_numeric_t numeric;
    ui_input_byte_t byte;
    ui_input_select_t select;
    ui_input_text_t text;
} ui_input_any_t;

/**
 * @brief Union type for input value storage.
 * Provides compact storage for different value types used by inputs.
 */
typedef union ui_input_value_any {
    bool bool_val;
    int int_val;
    uint8_t byte_val;
    void* ptr_val;
} ui_input_value_any_t;

#ifdef __cplusplus
}
#endif

#endif
