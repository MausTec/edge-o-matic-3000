#include "post_orgasm_control.h"

static const char* TAG = "post_orgasm_ruin";

volatile static struct {
    bool enabled;
    post_orgasm_mode_t post_orgasm_mode;
    event_handler_node_t* _h_post_orgasm_set;
    event_handler_node_t* _h_post_orgasm;
} post_orgasm_state;


static void _evt_post_orgasm_start(
    const char* event,
    EVENT_HANDLER_ARG_TYPE event_arg_ptr,
    int motor_speed,
    EVENT_HANDLER_ARG_TYPE handler_arg
) {
    if (post_orgasm_state.post_orgasm_mode == Ruin_orgasm){
        event_manager_dispatch(EVT_SPEED_CHANGE, NULL, 0 );
        if (!Config.orgasm_session_setup_ruin) {
            event_manager_dispatch(EVT_ORGASM_CONTROL_SHUTDOWN, NULL, 0);
        } else {
            // Restart session with choosing orgasm_trigger and post orgasm mode (best used with random)
            event_manager_dispatch(EVT_ORGASM_CONTROL_SESSION_SETUP, NULL, 0);
        }                
    }
}

static void _evt_post_orgasm_set(
    const char* evt, EVENT_HANDLER_ARG_TYPE eap, int post_orgasm_mode, EVENT_HANDLER_ARG_TYPE hap
) {
        post_orgasm_state.post_orgasm_mode = post_orgasm_mode;
}

void post_orgasm_ruin_init(void) {
    
    if (post_orgasm_state._h_post_orgasm_set == NULL) {
        post_orgasm_state._h_post_orgasm_set =
            event_manager_register_handler(EVT_ORGASM_CONTROL_POST_ORGASM_SET, &_evt_post_orgasm_set, NULL);
    }

    if (post_orgasm_state._h_post_orgasm == NULL) {
        post_orgasm_state._h_post_orgasm =
            event_manager_register_handler(EVT_ORGASM_CONTROL_POST_ORGASM_START, &_evt_post_orgasm_start, NULL);
    }
}