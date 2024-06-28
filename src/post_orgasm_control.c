#include "post_orgasm_control.h"
#include "orgasm_control.h"
#include "config.h"
#include "esp_log.h"
#include <math.h>

#include "post_orgasm_modules/post_orgasm_milk_o_matic.h"
#include "post_orgasm_modules/post_orgasm_default.h"
#include "post_orgasm_modules/post_orgasm_ruin.h"

static const char* TAG = "Post_orgasm_control";

/**
 *  Chooses a post_orgasm_mode if receiving random
 *  else returns the input 
 *  @param post_orgasm_mode
 *  @return chosen post_orgasm_mode
 */
post_orgasm_mode_t post_orgasm_mode_random_select(post_orgasm_mode_t post_orgasm_mode) {
    if (post_orgasm_mode != Random_mode) {
        return post_orgasm_mode;
    } else {

        // don't know how to set this up with dynamic modules since it needs the config variables
        int total_weight = Config.random_post_default_weight + Config.random_post_mom_weight + Config.random_post_ruin_weight;
        int random_value = rand() % total_weight;
        if ( random_value < Config.random_post_default_weight ) {
            return Default;
        } else if ( random_value < Config.random_post_default_weight + Config.random_post_mom_weight ) {
            return Milk_o_matic;
        } else {
            return Ruin_orgasm;
        }
    }
}


void post_orgasm_init(void) {
    post_orgasm_milk_o_matic_init();
    post_orgasm_default_init();
    post_orgasm_ruin_init();
}