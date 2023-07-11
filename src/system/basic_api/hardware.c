#include "basic_api.h"
#include <stddef.h>
#include "esp_log.h"

static const char *TAG = "basic:hardware";

void basic_api_register_hardware(struct mb_interpreter_t *bas) {
    mb_begin_module(bas, "HARDWARE");

    // Register namespace here.

    mb_end_module(bas);
}