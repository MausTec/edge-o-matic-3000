#include "orgasm_control.h"
#include "orgasm_trigger_control.h"
#include "config.h"
#include "esp_log.h"
#include <math.h>

#include "post_orgasm_modules/orgasm_trigger_timer.h"
#include "post_orgasm_modules/orgasm_trigger_edge_count.h"
#include "post_orgasm_modules/orgasm_trigger_now.h"

static const char* TAG = "Orgasm_trigger_control";

//extern session_t session_state;

/**
 *  Chooses a orgasm trigger if receiving random
 *  else returns the input 
 *  @param orgasm_trigger
 *  @return chosen orgasm_trigger
 */
orgasm_triggers_t orgasm_trigger_random_select(orgasm_triggers_t orgasm_trigger) {
    if (orgasm_trigger != Random_triggers) {
        return orgasm_trigger;
    } else {
        
        // don't know how to set this up with dynamic modules since it needs the config variables
        int total_weight = Config.random_trigger_now_weight + Config.random_trigger_timer_weight + Config.random_trigger_edge_count_weight;

        int random_value = rand() % total_weight;
        if ( random_value < Config.random_trigger_now_weight ) {
            return Now;
        } else if ( random_value < Config.random_trigger_now_weight + Config.random_trigger_timer_weight ) {
            return Timer;
        } else {
            return Edge_count;
        }
    }  
}

void orgasm_trigger_init() {
    orgasm_trigger_timer_init();
    orgasm_trigger_edge_count_init();
    orgasm_trigger_now_init();
}