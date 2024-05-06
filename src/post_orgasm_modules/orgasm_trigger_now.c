
//#include "orgasm_control.h"
//#include "system/event_manager.h"
//#include "config.h"
//#include <math.h>
#include "orgasm_trigger_control.h"

static const char* TAG = "orgasm_trigger_now";

//extern session_t session_state;

volatile static struct {
    bool enabled;
    event_handler_node_t* _h_orgasm_trigger;
    event_handler_node_t* _h_orgasm_trigger_set;
} orgasm_trigger_state = { 0 };

static void _evt_orgasm_trigger(
    const char* evt, EVENT_HANDLER_ARG_TYPE eap, int eai, EVENT_HANDLER_ARG_TYPE hap
) {
    if (orgasm_trigger_state.enabled) {
        event_manager_dispatch(EVT_ORGASM_CONTROL_ORGASM_IS_PERMITED, NULL, 0);
        orgasm_trigger_state.enabled = false;
    }
}

static void _evt_orgasm_trigger_set(
    const char* evt, EVENT_HANDLER_ARG_TYPE eap, int orgasm_trigger, EVENT_HANDLER_ARG_TYPE hap
) {
    if (orgasm_trigger == Now) {
        orgasm_trigger_state.enabled = true;
    } else {
        orgasm_trigger_state.enabled = false;
    }    
}



void orgasm_trigger_now_init(void) {
    orgasm_trigger_state.enabled = false;
    
    if (orgasm_trigger_state._h_orgasm_trigger_set == NULL) {
        orgasm_trigger_state._h_orgasm_trigger_set =
            event_manager_register_handler(EVT_ORGASM_CONTROL_ORGASM_TRIGGER_SET, &_evt_orgasm_trigger_set, NULL);
    }

    if (orgasm_trigger_state._h_orgasm_trigger == NULL) {
        orgasm_trigger_state._h_orgasm_trigger =
            event_manager_register_handler(EVT_ORGASM_CONTROL_IS_ORGASM_PERMITED, &_evt_orgasm_trigger, NULL);
    }
}


void orgasm_trigger_now_cleanup(void) {   
    if (orgasm_trigger_state._h_orgasm_trigger_set != NULL) {
        event_manager_unregister_handler(EVT_ORGASM_CONTROL_ORGASM_TRIGGER_SET, &_evt_orgasm_trigger_set);
    }

    if (orgasm_trigger_state._h_orgasm_trigger != NULL) {
            event_manager_unregister_handler(EVT_ORGASM_CONTROL_IS_ORGASM_PERMITED, &_evt_orgasm_trigger);
    }
}