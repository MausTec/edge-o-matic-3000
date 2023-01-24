#include "config.h"
#include "esp_log.h"
#include "orgasm_control.h"
#include "vibration_mode_controller.h"

static const char* TAG = "depletion_controller";

static struct {
    float motor_speed;
    uint16_t arousal;
    float base_speed;
} state;

static float start(void) {
    state.base_speed = Config.motor_start_speed;
    return state.base_speed;
}

static float increment(void) {
    if (state.base_speed < Config.motor_max_speed) {
        state.base_speed += calculate_increment(Config.motor_start_speed, Config.motor_max_speed,
                                                Config.motor_ramp_time_s);
    }

    float alter_perc = ((float)state.arousal / Config.sensitivity_threshold);
    float final_speed = state.base_speed * (1 - alter_perc);

    if (final_speed < (float)Config.motor_start_speed) {
        return Config.motor_start_speed;
    } else if (final_speed > (float)Config.motor_max_speed) {
        return Config.motor_max_speed;
    } else {
        return final_speed;
    }
}

static void tick(float motor_speed, uint16_t arousal) {
    state.motor_speed = motor_speed;
    state.arousal = arousal;
}

static float stop(void) { return 0; }

const vibration_mode_controller_t DepletionController = {
    .start = start,
    .increment = increment,
    .tick = tick,
    .stop = stop,
};