#include "application.h"
#include "basic_api.h"
#include "esp_log.h"
#include "ui/page.h"
#include "ui/ui.h"

static const char* TAG = "basic:application";

static int _start_foreground_app(struct mb_interpreter_t* s, void** l) {
    int result = MB_FUNC_OK;

    application_t* app = application_find_by_interpreter(s);

    ESP_LOGI(TAG, "Starting foreground application: %p", app);

    if (app == NULL) {
        return MB_FUNC_OK;
    }

    // Register interpreter context and launch application:
    app->interpreter_context = l;
    mb_value_t app_class;

    mb_check(mb_attempt_func_begin(s, l));
    mb_check(mb_attempt_func_end(s, l));

    mb_check(mb_get_routine(s, l, "LOOP", &app->fn_loop));
    ESP_LOGI(TAG, "fn_loop=%d", app->fn_loop.type);

    ui_open_page(&PAGE_APP_RUNNER, app);

    // Main app loop:
    for (;;) {
        mb_value_t ret;
        mb_value_t args[] = {};
        mb_check(mb_eval_routine(s, l, app->fn_loop, args, 0, &ret));

        if (ret.type != MB_DT_NIL) {
            ESP_LOGI(TAG, "fn loop returned=%d", ret.type);
            break;
        }

        vTaskDelay(1);
    }

    return MB_FUNC_OK;
}

void basic_api_register_application(struct mb_interpreter_t* bas) {
    mb_begin_module(bas, "APPLICATION");
    mb_register_func(bas, "START", _start_foreground_app);
    mb_end_module(bas);
}