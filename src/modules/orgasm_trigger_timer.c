#include "./orgasm_trigger_timer.h"
#include "orgasm_control.h"
#include "system/event_manager.h"
#include "config.h"
#include "esp_timer.h"
#include <math.h>

//static const char* TAG = "orgasm_trigger_timer";

volatile static struct {
    oc_bool_t enabled;
    // Autoedging Time and Post-Orgasm varables
    unsigned long edge_start_millis;
    unsigned long auto_edging_duration_minutes;
    event_handler_node_t* _h_orgasm_trigger_timer;
    event_handler_node_t* _h_orgasm_trigger_timer_start;
} orgasm_trigger_timer = { 0 };


static void _evt_orgasm_trigger_timer(
    const char* evt, EVENT_HANDLER_ARG_TYPE eap, int eai, EVENT_HANDLER_ARG_TYPE hap
) {
    if (orgasm_trigger_timer.enabled) {
        if ((esp_timer_get_time() / 1000UL) > (orgasm_trigger_timer.edge_start_millis +
                                               (orgasm_trigger_timer.auto_edging_duration_minutes * 60 * 1000))) {
            event_manager_dispatch(EVT_ORGASM_IS_PERMITED, NULL, 0);

        } 
    }
}

static void _evt_orgasm_trigger_timer_set(
    const char* evt, EVENT_HANDLER_ARG_TYPE eap, int eai, EVENT_HANDLER_ARG_TYPE hap
) {
    if (Config.orgasm_triggers == Timer) {

        orgasm_trigger_timer.enabled = ocTRUE;
        orgasm_trigger_timer.edge_start_millis = (esp_timer_get_time() / 1000UL);
        
        // random the trigger
        if (Config.random_orgasm_triggers) {
            // Only run once afer start of session
            int min_edge_time = round( Config.auto_edging_duration_minutes / 2 );
            orgasm_trigger_timer.auto_edging_duration_minutes = 
                min_edge_time + rand() % (Config.auto_edging_duration_minutes - min_edge_time );
        } else {
            orgasm_trigger_timer.auto_edging_duration_minutes = Config.auto_edging_duration_minutes;
        }

    } else {
        orgasm_trigger_timer.enabled = ocFALSE;
    }    
}



void orgasm_trigger_timer_init(void) {
    orgasm_trigger_timer.enabled = ocFALSE;
    
    if (orgasm_trigger_timer._h_orgasm_trigger_timer_start == NULL) {
        orgasm_trigger_timer._h_orgasm_trigger_timer_start =
            event_manager_register_handler(EVT_ORGASM_TRIGGER_SET, &_evt_orgasm_trigger_timer_set, NULL);
    }

    if (orgasm_trigger_timer._h_orgasm_trigger_timer == NULL) {
        orgasm_trigger_timer._h_orgasm_trigger_timer =
            event_manager_register_handler(EVT_IS_ORGASM_PERMITED, &_evt_orgasm_trigger_timer, NULL);
    }
}


void orgasm_trigger_timer_cleanup(void) {   
    if (orgasm_trigger_timer._h_orgasm_trigger_timer_start != NULL) {
        event_manager_unregister_handler(EVT_ORGASM_TRIGGER_SET, &_evt_orgasm_trigger_timer_set);
    }

    if (orgasm_trigger_timer._h_orgasm_trigger_timer != NULL) {
            event_manager_unregister_handler(EVT_IS_ORGASM_PERMITED, &_evt_orgasm_trigger_timer);
    }
}