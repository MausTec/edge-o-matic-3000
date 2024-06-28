#include "post_orgasm_control.h"

static const char* TAG = "post_orgasm_milk_o_matic";

volatile static struct {
    bool enabled;
    bool orgasm_permited;
    // Autoedging Time and Post-Orgasm varables
    unsigned long post_orgasm_start_millis;
    unsigned long post_orgasm_duration_millis;
    post_orgasm_mode_t post_orgasm_mode;
    event_handler_node_t* _h_post_orgasm_set;
    event_handler_node_t* _h_post_orgasm;
    event_handler_node_t* _h_orgasm;
    event_handler_node_t* _h_orgasm_is_permited;
    uint8_t orgasm_count;
} post_orgasm_state = { 0 };

volatile static struct {
    float motor_speed;
} output_state;

static void _evt_orgasm_is_permited(
    const char* evt, EVENT_HANDLER_ARG_TYPE eap, int eai, EVENT_HANDLER_ARG_TYPE hap
) {
    if (post_orgasm_state.post_orgasm_mode == Milk_o_matic ) {
        post_orgasm_state.orgasm_permited = true;
    }
}

static void _evt_orgasm_start(
    const char* event,
    EVENT_HANDLER_ARG_TYPE event_arg_ptr,
    int orgasm_count,
    EVENT_HANDLER_ARG_TYPE handler_arg
) { 
    if (post_orgasm_state.post_orgasm_mode == Milk_o_matic && 
            post_orgasm_state.orgasm_permited ) {
        post_orgasm_state.post_orgasm_start_millis = (esp_timer_get_time() / 1000UL);
        post_orgasm_state.post_orgasm_duration_millis =
                (Config.mom_post_orgasm_duration_seconds * 1000);
        post_orgasm_state.orgasm_count += 1;
        post_orgasm_state.orgasm_permited = false;
    } 
}


static void _evt_post_orgasm_start(
    const char* event,
    EVENT_HANDLER_ARG_TYPE event_arg_ptr,
    int motor_speed,
    EVENT_HANDLER_ARG_TYPE handler_arg
) {
    const vibration_mode_controller_t* post_orgasm_controller = &RampStopController;
    if (post_orgasm_state.enabled == false ) {
        // only set on first post_orgasm event
        output_state.motor_speed = motor_speed;
    }
    if (post_orgasm_state.post_orgasm_mode == Milk_o_matic){
        post_orgasm_state.enabled = true;
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
                                                        (Config.milk_o_matic_rest_duration_seconds * 1000)) {
                        // Rest period is finished. Reset variables for next round
                        post_orgasm_state.enabled = false;
                        event_manager_dispatch(EVT_ORGASM_CONTROL_SESSION_START, NULL, 0);
                    }
                } else {
                    if (!Config.orgasm_session_setup_m_o_m) {
                        // post orgasm modes has finished. Turn off everything and return to manual mode
                        post_orgasm_state.enabled = false;
                        post_orgasm_state.orgasm_count = 0;
                        event_manager_dispatch(EVT_ORGASM_CONTROL_SHUTDOWN, NULL, 0);
                    } else {
                        // Restart session with choosing orgasm_trigger and post orgasm mode (best used with random)
                        post_orgasm_state.enabled = false;
                        post_orgasm_state.orgasm_count = 0;
                        event_manager_dispatch(EVT_ORGASM_CONTROL_SESSION_SETUP, NULL, 0);
                    }
                }
            }
        }
    } else {
        post_orgasm_state.enabled = false;
        post_orgasm_state.orgasm_count = 0;
    }
}

static void _evt_post_orgasm_set(
    const char* evt, EVENT_HANDLER_ARG_TYPE eap, int post_orgasm_mode, EVENT_HANDLER_ARG_TYPE hap
) {
        post_orgasm_state.post_orgasm_mode = post_orgasm_mode;
}


void post_orgasm_milk_o_matic_init(void) {
    post_orgasm_state.enabled = false;
    post_orgasm_state.orgasm_count = 0;
    post_orgasm_state.orgasm_permited = false;
    
    if (post_orgasm_state._h_post_orgasm_set == NULL) {
        post_orgasm_state._h_post_orgasm_set =
            event_manager_register_handler(EVT_ORGASM_CONTROL_POST_ORGASM_SET, &_evt_post_orgasm_set, NULL);
    }

    if (post_orgasm_state._h_orgasm == NULL) {
        post_orgasm_state._h_orgasm =
            event_manager_register_handler(EVT_ORGASM_START, &_evt_orgasm_start, NULL);
    }
    if (post_orgasm_state._h_post_orgasm == NULL) {
        post_orgasm_state._h_post_orgasm =
            event_manager_register_handler(EVT_ORGASM_CONTROL_POST_ORGASM_START, &_evt_post_orgasm_start, NULL);
    }
    if (post_orgasm_state._h_orgasm_is_permited == NULL) {
        post_orgasm_state._h_orgasm_is_permited =
            event_manager_register_handler(EVT_ORGASM_CONTROL_ORGASM_IS_PERMITED, &_evt_orgasm_is_permited, NULL);
    }    
    
}