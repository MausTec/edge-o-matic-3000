#ifndef __orgasm_trigger_control_h
#define __orgasm_trigger_control_h

#ifdef __cplusplus
extern "C" {
#endif

//#include "orgasm_control.h"
#include "system/event_manager.h"
#include "config.h"
#include <math.h>

// Orgasm Trigger
typedef enum orgasm_triggers {
    Timer,
    Edge_count,
    Now,
    Random_triggers
} orgasm_triggers_t;

void orgasm_trigger_init(void);
orgasm_triggers_t orgasm_trigger_random_select(orgasm_triggers_t);

#ifdef __cplusplus
}
#endif

#endif
