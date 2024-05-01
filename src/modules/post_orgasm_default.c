#include "./post_orgasm_default.h"

#include "orgasm_control.h"
#include "system/event_manager.h"
#include "config.h"
#include "esp_timer.h"
#include "eom-hal.h"

static const char* TAG = "post_orgasm_default";

volatile static struct {
    oc_bool_t enabled;
    // Autoedging Time and Post-Orgasm varables
    unsigned long post_orgasm_start_millis;
    unsigned long post_orgasm_duration_millis;
    event_handler_node_t* _h_post_orgasm;
    event_handler_node_t* _h_orgasm;
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
    if (Config.post_orgasm_mode == Default){
        if (post_orgasm_state.enabled == ocFALSE) {
            post_orgasm_state.post_orgasm_start_millis = (esp_timer_get_time() / 1000UL);
            post_orgasm_state.post_orgasm_duration_millis =
                    (Config.post_orgasm_duration_seconds * 1000);
        }
    }
}



static void _evt_post_orgasm_start(
    const char* event,
    EVENT_HANDLER_ARG_TYPE event_arg_ptr,
    int motor_speed,
    EVENT_HANDLER_ARG_TYPE handler_arg
) {
    const vibration_mode_controller_t* Post_orgasm_controller = &RampStopController;
    if (post_orgasm_state.enabled == ocFALSE) {
        output_state.motor_speed = motor_speed;
    }
    if (Config.post_orgasm_mode == Default){
        post_orgasm_state.enabled = ocTRUE;
        Post_orgasm_controller->tick(output_state.motor_speed, 0);
        eom_hal_set_encoder_rgb(255, 0, 0);

        // Detect if within post orgasm session
        if ((esp_timer_get_time() / 1000UL) < (post_orgasm_state.post_orgasm_start_millis +
                                               post_orgasm_state.post_orgasm_duration_millis)) {
            // continue to raise motor to max speed
            output_state.motor_speed = Post_orgasm_controller->increment();
            event_manager_dispatch(EVT_SPEED_CHANGE, NULL, output_state.motor_speed);

        } else {                                // Post_orgasm timer reached
            if (motor_speed > 0) { // Ramp down motor speed to 0
                event_manager_dispatch(EVT_SPEED_CHANGE, NULL, motor_speed -1 );
            } else {
                // post orgasm modes has finished. Turn off everything and return to manual mode
                post_orgasm_state.enabled = ocFALSE;
                event_manager_dispatch(EVT_ORGASM_CONTROL_SHUTDOWN, NULL, 0);
            }
        }
    }
}

void post_orgasm_default_init(void) {
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