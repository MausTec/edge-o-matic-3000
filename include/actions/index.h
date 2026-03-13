#ifndef __actions__index_h
#define __actions__index_h

#ifdef __cplusplus
extern "C" {
#endif

#include "mt_actions.h"

void actions_register_system(void);
void action_config_init(void);
void action_ble_init(void);

static inline void actions_register_all(void) {
    actions_register_system();
    action_config_init();
    action_ble_init();

    // Event registration: Our event handler mapps ALL events to mta, but we should be a bit more
    // explicit. For now, let's just register all known events.

    // System events (dispatched via action_manager_dispatch_event / EVT_*)
    mta_register_event("modeSet", NULL);
    mta_register_event("speedChange", NULL);
    mta_register_event("arousalChange", "arousal:read");
    mta_register_event("orgasmDenial", "arousal:read");
    mta_register_event("edgeStart", "arousal:read");
    mta_register_event("orgasmStart", "arousal:read");

    // BLE driver events (emitted directly by plugin_driver.c)
    mta_register_event("connect", NULL);
    mta_register_event("disconnect", NULL);

    // TODO: This should become a built-in event in mt-actions core so that any
    // host embedding mt-actions gets it for free.
    mta_register_event("tick", NULL);
}

#ifdef __cplusplus
}
#endif

#endif
