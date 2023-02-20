#include "application.h"
#include "basic_api.h"
#include "esp_log.h"
#include "ui/page.h"
#include "ui/ui.h"

static const char* TAG = "basic:application";

static int _start_foreground_app(struct mb_interpreter_t* s, void** l) {
    application_t* app = application_find_by_interpreter(s);

    ESP_LOGI(TAG, "Starting foreground application: %p", app);

    if (app == NULL) {
        return MB_FUNC_OK;
    }

    // Register interpreter context and launch application:
    app->interpreter_context = l;
    ui_open_page(&PAGE_APP_RUNNER, app);

    return MB_FUNC_OK;
}

void basic_api_register_application(struct mb_interpreter_t* bas) {
    mb_begin_module(bas, "APPLICATION");

    mb_register_func(bas, "START", _start_foreground_app);

    mb_end_module(bas);
}