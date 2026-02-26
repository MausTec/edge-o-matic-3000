#include "config.h"
#include "esp_log.h"
#include "vibration_mode_controller.h"

static const char* TAG = "enhancement_controller";

static struct {
    float motor_speed;
    uint16_t arousal;
} state;

/**
 * Enhancement mode: positive feedback loop with arousal. Motor speed scales
 * proportionally with arousal, building toward orgasm rather than preventing it.
 * Intended for "orgasm allowed" mode in future releases.
 */

static float start(void) {
    return Config.motor_min_speed;
}

static float increment(void) {
    if (Config.sensitivity_threshold == 0) return 1.0;

    float speed_diff = Config.motor_max_speed - Config.motor_min_speed;
    float alter_perc = ((float)state.arousal / Config.sensitivity_threshold);
    return Config.motor_min_speed + (alter_perc * speed_diff);
}

static void tick(float motor_speed, uint16_t arousal) {
    state.motor_speed = motor_speed;
    state.arousal = arousal;
}

static float stop(void) {
    return 0;
}

static float on_edge(void) {
    return Config.motor_max_speed;
}

const vibration_mode_controller_t EnhancementController = {
    .start = start,
    .increment = increment,
    .stop = stop,
    .on_edge = on_edge,
    .tick = tick,
};