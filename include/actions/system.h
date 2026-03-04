#ifndef __actions__system_h
#define __actions__system_h

#ifdef __cplusplus
extern "C" {
#endif

#include "mt_actions.h"

int action_system_delay(
    struct mta_plugin* plugin, mta_scope_t* scope, mta_arg_t* args, uint8_t arg_count
);

#ifdef __cplusplus
}
#endif

#endif
