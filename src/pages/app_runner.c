#include "application.h"
#include "esp_log.h"
#include "ui/graphics.h"
#include "ui/page.h"
#include "util/i18n.h"

static const char* TAG = "page:app_runner";

static void on_open(void* arg) {
    application_t* app = (application_t*)arg;
    if (app == NULL) {
        // no app
        return;
    }

    ESP_LOGI(TAG, "Page opened! App: %s", app->title);
}

static void on_render(u8g2_t* d, void* arg) {
    application_t* app = (application_t*)arg;

    if (app == NULL) {
        ui_draw_status(d, _("NO APP"));
        return;
    }

    ui_draw_status(d, app->title);
}

static void on_close(void* arg) {
    application_t* app = (application_t*)arg;
    application_kill(app);
}

const struct ui_page PAGE_APP_RUNNER = {
    .title = "Application",
    .on_open = on_open,
    .on_close = on_close,
    .on_render = on_render,
};