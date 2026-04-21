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
     * @event mode_set
     * @module system
     */
    mta_register_event("mode_set", NULL);

    /**
     * Fires when the motor speed changes.
     * @event speed_change
     * @module system
     * @payload speed:int New speed value (0-255)
     */
    mta_register_event("speed_change", NULL);

    /**
     * Fires when the arousal level changes.
     * @event arousal_change
     * @module arousal
     * @payload level:int Current arousal percentage (0-100)
     */
    mta_register_event("arousal_change", "arousal:read");

    /**
     * Fires when an orgasm is denied (edge detected).
     * @event orgasm_denial
     * @module arousal
     * @payload level:int Arousal level at denial
     */
    mta_register_event("orgasm_denial", "arousal:read");

    /**
     * Fires when edging starts (near-orgasm detection begins).
     * @event edge_start
     * @module arousal
     */
    mta_register_event("edge_start", "arousal:read");

    /**
     * Fires when an orgasm begins.
     * @event orgasm_start
     * @module arousal
     */
    mta_register_event("orgasm_start", "arousal:read");

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
