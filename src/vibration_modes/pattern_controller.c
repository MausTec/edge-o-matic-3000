#include "config.h"
#include "esp_log.h"
#include "orgasm_control.h"
#include "vibration_mode_controller.h"

static const char* TAG = "pattern_controller";

static struct {
    float motor_speed;
    uint16_t arousal;
    size_t pattern_step;
    uint32_t step_ticks;
    vibration_pattern_t* pattern;
} state;

// This is unfinished.
// We'll use it some time I think.

static float increment(void);

static float start(void) {
    state.pattern_step = 0;
    state.step_ticks = 0;
    return increment();
}

static float increment(void) { return 0.0f; }

static void tick(float motor_speed, uint16_t arousal) {
    state.motor_speed = motor_speed;
    state.arousal = arousal;
}

static float stop(void) { return 0; }

const vibration_mode_controller_t PatternController = {
    .start = start,
    .increment = increment,
    .tick = tick,
    .stop = stop,
};