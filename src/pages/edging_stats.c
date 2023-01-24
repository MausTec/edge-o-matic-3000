#include "ui/page.h"

static void on_open(void* arg) {}

static ui_render_flag_t on_loop(void* arg) { return NORENDER; }

static void on_render(u8g2_t* d, void* arg) {}

static void on_exit(void* arg) {}

const struct ui_page PAGE_EDGING_STATS = {
    .title = "Edging Stats",
    .on_open = on_open,
};