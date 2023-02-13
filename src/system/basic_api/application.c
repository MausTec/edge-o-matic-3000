#include "basic_api.h"
#include "esp_log.h"

static const char* TAG = "basic:application";

static int _register_application(struct mb_interpreter_t* s, void** l) {
}

void basic_api_register_application(struct mb_interpreter_t* bas) {
    mb_begin_module(bas, "APPLICATION");

    mb_register_func(bas, "REGISTER_APPLICATION", _register_application);

    mb_end_module(bas);
}