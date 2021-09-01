#ifndef __eom_tscode_handler_h
#define __eom_tscode_handler_h

#ifdef __cplusplus
extern "C" {
#endif

#include "tscode.h"
#include <stddef.h>

tscode_command_response_t eom_tscode_handler(tscode_command_t* cmd, char* response, size_t resp_len);

#ifdef __cplusplus
}
#endif

#endif