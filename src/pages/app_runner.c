#include "application.h"
#include "esp_log.h"
#include "ui/page.h"

static const char* TAG = "page:app_runner";

static void on_open(void* arg) {
    application_t* app = (application_t*)arg;
    application_start(app);
}

static void on_close(void* arg) {
}

const struct ui_page PAGE_APP_RUNNER = {
    .title = "Application",
    .on_open = on_open,
    .on_close = on_close,
};