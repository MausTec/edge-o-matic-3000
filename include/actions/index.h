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

    /**
     * Fires when the active operating mode changes.
     * @event modeSet
     * @module system
     */
    mta_register_event("modeSet", NULL);

    /**
     * Fires when the motor speed changes.
     * @event speedChange
     * @module system
     * @payload speed:int New speed value (0-255)
     */
    mta_register_event("speedChange", NULL);

    /**
     * Fires when the arousal level changes.
     * @event arousalChange
     * @module arousal
     * @payload level:int Current arousal percentage (0-100)
     */
    mta_register_event("arousalChange", "arousal:read");

    /**
     * Fires when an orgasm is denied (edge detected).
     * @event orgasmDenial
     * @module arousal
     * @payload level:int Arousal level at denial
     */
    mta_register_event("orgasmDenial", "arousal:read");

    /**
     * Fires when edging starts (near-orgasm detection begins).
     * @event edgeStart
     * @module arousal
     */
    mta_register_event("edgeStart", "arousal:read");

    /**
     * Fires when an orgasm begins.
     * @event orgasmStart
     * @module arousal
     */
    mta_register_event("orgasmStart", "arousal:read");

    // BLE driver events (emitted directly by plugin_driver.c)

    /**
     * Fires when a BLE peripheral connects.
     * @event connect
     * @module ble
     */
    mta_register_event("connect", NULL);

    /**
     * Fires when a BLE peripheral disconnects.
     * @event disconnect
     * @module ble
     */
    mta_register_event("disconnect", NULL);

    // TODO: This should become a built-in event in mt-actions core so that any
    // host embedding mt-actions gets it for free.

    /**
     * Fires on each main loop tick (periodic heartbeat).
     * @event tick
     * @module system
     */
    mta_register_event("tick", NULL);
}

#ifdef __cplusplus
}
#endif

#endif
