#include "application.h"
#include "esp_log.h"
#include "ui/graphics.h"
#include "ui/page.h"
#include "ui/toast.h"
#include "util/i18n.h"

static const char* TAG = "page:app_runner";

static void on_open(void* arg) {
    application_t* app = (application_t*)arg;
    ESP_LOGI(TAG, "Opening application.");

    if (app == NULL) {
        ui_open_page(NULL, NULL);
        ui_toast("%s", _("Application not loaded."));
        return;
    }
}

static void on_render(u8g2_t* d, void* arg) {
    application_t* app = (application_t*)arg;

    if (app == NULL) {
        ui_draw_status(d, _("NO APP"));
        return;
    }

    ui_draw_status(d, app->title);
}

ui_render_flag_t on_loop(void* arg) {
    if (!arg) return NORENDER;
    application_t* app = (application_t*)arg;

    mb_value_t ret;
    mb_value_t args[] = {};

    mb_check(
        mb_eval_routine(app->interpreter, app->interpreter_context, app->fn_loop, args, 0, &ret)
    );

    return PASS;
}

static void on_close(void* arg) {
    application_t* app = (application_t*)arg;
    ESP_LOGI(TAG, "Closing application.");
    application_kill(app);
}

const struct ui_page PAGE_APP_RUNNER = {
    .title = "Application",
    .on_open = on_open,
    .on_close = on_close,
    .on_render = on_render,
    .on_loop = on_loop,
};