#include "VibrationModeController.h"
#include "config.h"
float VibrationModeController::rampToIncrement(int start, int target, float time_s) {
  if (time_s == 0) {
    return target;
  }

  float motor_increment = (
      (float)(target - start)  /
      ((float)Config.update_frequency_hz * (float)(time_s < 0 ? Config.motor_ramp_time_s : time_s))
  );

  return motor_increment;
}

void VibrationModeController::tick(float motor_speed, long arousal) {
  this->motor_speed = motor_speed;
  this->arousal = arousal;
}

/**
 * RampStop Controller
 */
float RampStopController::start() {
  return Config.motor_start_speed;
}

float RampStopController::increment() {
  // Motor increment goes 0 - 100 in ramp_time_s, in steps of 1/update_fequency
  float motor_increment = rampToIncrement(Config.motor_start_speed, Config.motor_max_speed);

  if (this->motor_speed < (Config.motor_max_speed - motor_increment)) {
    return this->motor_speed + motor_increment;
  } else {
    return Config.motor_max_speed;
  }
}

/**
 * Enhancement Controller
 */
float EnhancementController::start() {
  return Config.motor_start_speed;
}

float EnhancementController::increment() {
  if (stopped) {
    return this->motor_speed + rampToIncrement(Config.motor_max_speed, 0, Config.edge_delay);
  }

  float speed_diff = Config.motor_max_speed - Config.motor_start_speed;
  float alter_perc = ((float) this->arousal / Config.sensitivity_threshold);
  return Config.motor_start_speed + (alter_perc * speed_diff);
}

float EnhancementController::stop() {
  this->stopped = true;
  return Config.motor_max_speed;
}

/**
 * Depletion Controller
 */
float DepletionController::start() {
  this->base_speed = Config.motor_start_speed;
  return Config.motor_start_speed;
}

float DepletionController::increment() {
  if (this->base_speed < Config.motor_max_speed)
    this->base_speed = this->base_speed + rampToIncrement(Config.motor_start_speed, Config.motor_max_speed);

  float alter_perc = ((float) this->arousal / Config.sensitivity_threshold);
  float final_speed = base_speed * (1 - alter_perc);

  if (final_speed < (float)Config.motor_start_speed) {
    return Config.motor_start_speed;
  } else if (final_speed > (float)Config.motor_max_speed) {
    return Config.motor_max_speed;
  } else {
    return final_speed;
  }
}

/**
 * Pattern Controller
 */
VibrationPattern VibrationPatterns::Step[] = {
    {   0, 60, false },
    {  64, 60, false },
    { 128, 60, false },
    { 194, 60, false },
    { 255, 60, false },
    { 255, 60, true },
};

VibrationPattern VibrationPatterns::Wave[] = {
    {  64, 60, true },
    { 255, 30, true },
    { 128, 30, true },
    { 255, 60, true },
};

PatternController::PatternController() {
  setPattern(VibrationPatterns::Wave);
}

float PatternController::start() {
  this->pattern_step = 0;
  this->step_ticks = 0;
  return this->increment();
}

float PatternController::increment() {
  this->step_ticks++;
  VibrationPattern p = this->pattern[this->pattern_step];

  // Advance to the next step:
  if (this->step_ticks > p.hold_ticks) {
    this->pattern_step = (this->pattern_step + 1) % this->pattern_length;
    this->step_ticks = 0;
    p = this->pattern[this->pattern_step];
    return p.motor_speed;
  }

  VibrationPattern next = this->nextStep();

  if (p.ramp_to) {
    return motor_speed + rampToIncrement(p.motor_speed, next.motor_speed, p.hold_ticks / Config.update_frequency_hz);
  } else {
    return p.motor_speed;
  }
}

VibrationPattern PatternController::nextStep() {
  int step = (this->pattern_step + 1) % this->pattern_length;
  return this->pattern[step];
}

template <int N>
void PatternController::setPattern(VibrationPattern (&pattern)[N]) {
  log_i("Setting a pattern of %d steps.", sizeof(pattern) / sizeof(pattern[0]));
  this->pattern = pattern;
  this->pattern_length = sizeof(pattern) / sizeof(pattern[0]);
}

/**
 * Declare Instances of VibrationControllers
 */
RampStopController VibrationControllers::RampStop;
EnhancementController VibrationControllers::Enhancement;
DepletionController VibrationControllers::Depletion;
PatternController VibrationControllers::Pattern;