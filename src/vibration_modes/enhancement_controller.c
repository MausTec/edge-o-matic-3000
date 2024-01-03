#include "config.h"
#include "esp_log.h"
#include "orgasm_control.h"
#include "vibration_mode_controller.h"

static const char* TAG = "enhancement_controller";

static struct {
    float motor_speed;
    uint16_t arousal;
    oc_bool_t stopped;
} state;

static float start(void) {
    return Config.motor_start_speed;
}

static float increment(void) {
    if (Config.sensitivity_threshold == 0) return 1.0;

    if (state.stopped) {
        return state.motor_speed +
               calculate_increment(Config.motor_max_speed, 0, Config.edge_delay);
    }

    float speed_diff = Config.motor_max_speed - Config.motor_start_speed;
    float alter_perc = ((float)state.arousal / Config.sensitivity_threshold);
    return Config.motor_start_speed + (alter_perc * speed_diff);
}

static void tick(float motor_speed, uint16_t arousal) {
    state.motor_speed = motor_speed;
    state.arousal = arousal;
}

static float stop(void) {
    state.stopped = ocTRUE;
    return Config.motor_max_speed;
}

const vibration_mode_controller_t EnhancementController = {
    .start = start,
    .increment = increment,
    .tick = tick,
    .stop = stop,
};