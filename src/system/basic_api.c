#include "basic_api.h"

void basic_api_register_all(struct mb_interpreter_t* bas) {
    basic_api_register_ui(bas);
    basic_api_register_hardware(bas);
    basic_api_register_system(bas);
    basic_api_register_graphics(bas);
    basic_api_register_error_handlers(bas);
    basic_api_register_application(bas);
}

void basic_api_register_error_handlers(struct mb_interpreter_t* bas) {
}