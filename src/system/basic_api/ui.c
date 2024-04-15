#include "basic_api.h"
#include "esp_log.h"
#include "ui/toast.h"
#include <assert.h>
#include <stddef.h>

static const char* TAG = "basic:ui";

static int _toast(struct mb_interpreter_t* s, void** l) {
    int result = MB_FUNC_OK;
    mb_value_t arg;
    char* str = NULL;

    mb_assert(s && l);
    mb_check(mb_attempt_open_bracket(s, l));
    mb_check(mb_pop_value(s, l, &arg));
    mb_check(mb_attempt_close_bracket(s, l));

    switch (arg.type) {
    case MB_DT_NIL: break;

    case MB_DT_INT:
        ESP_LOGI(TAG, "Need to toast: %d", arg.value.integer);
        ui_toast("%d", arg.value.integer);
        break;

    case MB_DT_STRING:
        ESP_LOGI(TAG, "Need to toast: %s", arg.value.string);
        ui_toast("%s", arg.value.string);
        break;

    default: break;
    }

    mb_check(mb_push_int(s, l, 0));
    return result;
}

void basic_api_register_ui(struct mb_interpreter_t* bas) {
    mb_begin_module(bas, "UI");

    // Register namespace here.
    mb_register_func(bas, "TOAST", _toast);

    mb_end_module(bas);
}