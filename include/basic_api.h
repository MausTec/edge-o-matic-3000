#ifndef __edge_o_matic__basic_api_h
#define __edge_o_matic__basic_api_h

#ifdef __cplusplus
extern "C" {
#endif

#include "my_basic.h"

// Be sure to forward declare all namespaces here:
void basic_api_register_ui(struct mb_interpreter_t* bas);
void basic_api_register_hardware(struct mb_interpreter_t* bas);
void basic_api_register_system(struct mb_interpreter_t* bas);
void basic_api_register_graphics(struct mb_interpreter_t* bas);
void basic_api_register_application(struct mb_interpreter_t* bas);

// And some core API calls:
void basic_api_register_error_handlers(struct mb_interpreter_t* bas);
void basic_api_register_all(struct mb_interpreter_t* bas);

#ifdef __cplusplus
}
#endif

#endif
