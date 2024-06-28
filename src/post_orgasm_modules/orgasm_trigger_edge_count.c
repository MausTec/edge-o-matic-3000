#include "orgasm_trigger_control.h"

static const char* TAG = "orgasm_trigger_edge_count";


volatile static struct {
    bool enabled;
    int edge_count_max;
    int edge_count;
    event_handler_node_t* _h_edge_count;
    event_handler_node_t* _h_orgasm_trigger;
    event_handler_node_t* _h_orgasm_trigger_set;
} orgasm_trigger_state = { 0 };

static void _evt_orgasm_denial(
    const char* evt, EVENT_HANDLER_ARG_TYPE eap, int eai, EVENT_HANDLER_ARG_TYPE hap
) {
    if (orgasm_trigger_state.enabled) {
        orgasm_trigger_state.edge_count += 1;
    }
}

static void _evt_orgasm_trigger(
    const char* evt, EVENT_HANDLER_ARG_TYPE eap, int eai, EVENT_HANDLER_ARG_TYPE hap
) {
    if (orgasm_trigger_state.enabled) {
        if (orgasm_trigger_state.edge_count >= orgasm_trigger_state.edge_count_max) {
            event_manager_dispatch(EVT_ORGASM_CONTROL_ORGASM_IS_PERMITED, NULL, 0);
            orgasm_trigger_state.enabled = false;
            orgasm_trigger_state.edge_count = 0;
        } 
    }
}

static void _evt_orgasm_trigger_set(
    const char* evt, EVENT_HANDLER_ARG_TYPE eap, int orgasm_trigger, EVENT_HANDLER_ARG_TYPE hap
) {
    if (orgasm_trigger == Edge_count) {
        orgasm_trigger_state.enabled = true;
        orgasm_trigger_state.edge_count_max = Config.max_edge_count + orgasm_trigger_state.edge_count;

        // random the trigger
        if (Config.random_orgasm_triggers) {
            // Only run once afer start of session
            orgasm_trigger_state.edge_count_max =
                Config.min_edge_count + rand() % (Config.max_edge_count - Config.min_edge_count );
        }

    } else {
        orgasm_trigger_state.enabled = false;
        orgasm_trigger_state.edge_count = 0 ;
    }    
}



void orgasm_trigger_edge_count_init(void) {
    orgasm_trigger_state.enabled = false;
    orgasm_trigger_state.edge_count = 0;
    
    if (orgasm_trigger_state._h_orgasm_trigger_set == NULL) {
        orgasm_trigger_state._h_orgasm_trigger_set =
            event_manager_register_handler(EVT_ORGASM_CONTROL_ORGASM_TRIGGER_SET, &_evt_orgasm_trigger_set, NULL);
    }

    if (orgasm_trigger_state._h_edge_count == NULL) {
        orgasm_trigger_state._h_edge_count =
            event_manager_register_handler(EVT_ORGASM_DENIAL, &_evt_orgasm_denial, NULL);
    }

    if (orgasm_trigger_state._h_orgasm_trigger == NULL) {
        orgasm_trigger_state._h_orgasm_trigger =
            event_manager_register_handler(EVT_ORGASM_CONTROL_IS_ORGASM_PERMITED, &_evt_orgasm_trigger, NULL);
    }
}


void orgasm_trigger_edge_count_cleanup(void) {   
    if (orgasm_trigger_state._h_orgasm_trigger_set != NULL) {
        event_manager_unregister_handler(EVT_ORGASM_CONTROL_ORGASM_TRIGGER_SET, &_evt_orgasm_trigger_set);
    }

    if (orgasm_trigger_state._h_edge_count != NULL) {
            event_manager_unregister_handler(EVT_ORGASM_DENIAL, &_evt_orgasm_denial);
    }
    
    if (orgasm_trigger_state._h_orgasm_trigger != NULL) {
            event_manager_unregister_handler(EVT_ORGASM_CONTROL_IS_ORGASM_PERMITED, &_evt_orgasm_trigger);
    }
}