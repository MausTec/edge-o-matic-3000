#include "./post_orgasm_milk_o_matic.h"

#include "orgasm_control.h"
#include "system/event_manager.h"
#include "config.h"
#include "esp_timer.h"
#include "eom-hal.h"
#include "vibration_mode_controller.h"

static const char* TAG = "post_orgasm_milk_o_matic";

volatile static struct {
    oc_bool_t enabled;
    // Autoedging Time and Post-Orgasm varables
    unsigned long post_orgasm_start_millis;
    unsigned long post_orgasm_duration_millis;
    event_handler_node_t* _h_post_orgasm;
    event_handler_node_t* _h_orgasm;
    uint8_t orgasm_count;
} post_orgasm_state = { 0 };

volatile static struct {
    float motor_speed;
} output_state;


static void _evt_orgasm_start(
    const char* event,
    EVENT_HANDLER_ARG_TYPE event_arg_ptr,
    int orgasm_count,
    EVENT_HANDLER_ARG_TYPE handler_arg
) { 
    if (Config.post_orgasm_mode == Milk_o_matic && post_orgasm_state.enabled == ocFALSE) {
        post_orgasm_state.post_orgasm_start_millis = (esp_timer_get_time() / 1000UL);
        post_orgasm_state.post_orgasm_duration_millis =
                (Config.post_orgasm_duration_seconds * 1000);
        post_orgasm_state.orgasm_count = orgasm_count;
    }
}



static void _evt_post_orgasm_start(
    const char* event,
    EVENT_HANDLER_ARG_TYPE event_arg_ptr,
    int motor_speed,
    EVENT_HANDLER_ARG_TYPE handler_arg
) {
    const vibration_mode_controller_t* post_orgasm_controller = &RampStopController;
    if (post_orgasm_state.enabled == ocFALSE ) {
        // only set on first post_orgasm event
        output_state.motor_speed = motor_speed;
    }
    if (Config.post_orgasm_mode == Milk_o_matic){
        post_orgasm_state.enabled = ocTRUE;
        post_orgasm_controller->tick(output_state.motor_speed, 0);
        eom_hal_set_encoder_rgb(255, 0, 0);

        // Detect if within post orgasm session
        if ((esp_timer_get_time() / 1000UL) < (post_orgasm_state.post_orgasm_start_millis +
                                               post_orgasm_state.post_orgasm_duration_millis)) {
            // continue to raise motor to max speed
            output_state.motor_speed = post_orgasm_controller->increment();
            event_manager_dispatch(EVT_SPEED_CHANGE, NULL, output_state.motor_speed);

        } else {                                // Post_orgasm timer reached
            if (motor_speed > 0) { // Ramp down motor speed to 0
                event_manager_dispatch(EVT_SPEED_CHANGE, NULL, motor_speed - 1 );
            } else {
                // make sure you don't go over max orgasm count in milk-o-matic mode
                if ( post_orgasm_state.orgasm_count < Config.max_orgasms ) {
                        
                    // The motor has been turnned off but this is Milk-O-Matic orgasm mode. Prepare for next round.
                    // now show that your in milk-o-matic mode. Usefull if you choose random post orgasm mode, you only find out after first post orgasm
                    eom_hal_set_encoder_rgb(255, 0, 127);
                    // now give a break before restarting edging
                    if (esp_timer_get_time() / 1000UL > post_orgasm_state.post_orgasm_start_millis +
                                                        post_orgasm_state.post_orgasm_duration_millis +
                                                        (Config.milk_o_matic_rest_duration_minutes * 60 * 1000)) {
                        // Rest period is finished. Reset variables for next round
                        post_orgasm_state.enabled = ocFALSE;
                        event_manager_dispatch(EVT_ORGASM_CONTROL_RESTART, NULL, 0);
                    }
                } else {
                    // post orgasm modes has finished. Turn off everything and return to manual mode
                    post_orgasm_state.enabled = ocFALSE;
                    event_manager_dispatch(EVT_ORGASM_CONTROL_SHUTDOWN, NULL, 0);
                }
            }
        }
    }
}

void post_orgasm_milk_o_matic_init(void) {
    post_orgasm_state.enabled = ocFALSE;

    if (post_orgasm_state._h_orgasm == NULL) {
        post_orgasm_state._h_orgasm =
            event_manager_register_handler(EVT_ORGASM_START, &_evt_orgasm_start, NULL);
    }
    if (post_orgasm_state._h_post_orgasm == NULL) {
        post_orgasm_state._h_post_orgasm =
            event_manager_register_handler(EVT_POST_ORGASM_START, &_evt_post_orgasm_start, NULL);
    }
}