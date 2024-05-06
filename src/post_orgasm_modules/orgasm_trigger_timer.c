#include "orgasm_control.h"
#include "system/event_manager.h"
#include "config.h"
#include "esp_timer.h"
#include <math.h>
#include "orgasm_trigger_control.h"

//static const char* TAG = "orgasm_trigger_timer";

//extern session_t session_state;

volatile static struct {
    bool enabled;
    // Autoedging Time and Post-Orgasm varables
    unsigned long edge_start_millis;
    unsigned long edging_duration_seconds;
    event_handler_node_t* _h_orgasm_trigger;
    event_handler_node_t* _h_orgasm_trigger_set;
} orgasm_trigger_state = { 0 };

static void _evt_orgasm_trigger_timer(
    const char* evt, EVENT_HANDLER_ARG_TYPE eap, int eai, EVENT_HANDLER_ARG_TYPE hap
) {
    if (orgasm_trigger_state.enabled) {
        if ((esp_timer_get_time() / 1000UL) > (orgasm_trigger_state.edge_start_millis +
                                               (orgasm_trigger_state.edging_duration_seconds * 1000))) {
            event_manager_dispatch(EVT_ORGASM_CONTROL_ORGASM_IS_PERMITED, NULL, 0);
            orgasm_trigger_state.enabled = false;
        } 
    }
}

static void _evt_orgasm_trigger_timer_set(
    const char* evt, EVENT_HANDLER_ARG_TYPE eap, int orgasm_trigger, EVENT_HANDLER_ARG_TYPE hap
) {
    if (orgasm_trigger == Timer) {

        orgasm_trigger_state.enabled = true;
        orgasm_trigger_state.edge_start_millis = (esp_timer_get_time() / 1000UL);
        
        // random the trigger
        if (Config.random_orgasm_triggers) {
            // Only run once afer start of session
            orgasm_trigger_state.edging_duration_seconds = 
                Config.min_timer_seconds + rand() % (Config.max_timer_seconds - Config.min_timer_seconds );
        } else {
            orgasm_trigger_state.edging_duration_seconds = Config.max_timer_seconds;
        }

    } else {
        orgasm_trigger_state.enabled = false;
    }    
}



void orgasm_trigger_timer_init(void) {
    orgasm_trigger_state.enabled = false;
    
    if (orgasm_trigger_state._h_orgasm_trigger_set == NULL) {
        orgasm_trigger_state._h_orgasm_trigger_set =
            event_manager_register_handler(EVT_ORGASM_CONTROL_ORGASM_TRIGGER_SET, &_evt_orgasm_trigger_timer_set, NULL);
    }

    if (orgasm_trigger_state._h_orgasm_trigger == NULL) {
        orgasm_trigger_state._h_orgasm_trigger =
            event_manager_register_handler(EVT_ORGASM_CONTROL_IS_ORGASM_PERMITED, &_evt_orgasm_trigger_timer, NULL);
    }
}


void orgasm_trigger_timer_cleanup(void) {   
    if (orgasm_trigger_state._h_orgasm_trigger_set != NULL) {
        event_manager_unregister_handler(EVT_ORGASM_CONTROL_ORGASM_TRIGGER_SET, &_evt_orgasm_trigger_timer_set);
    }

    if (orgasm_trigger_state._h_orgasm_trigger != NULL) {
            event_manager_unregister_handler(EVT_ORGASM_CONTROL_IS_ORGASM_PERMITED, &_evt_orgasm_trigger_timer);
    }
}