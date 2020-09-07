#include "../include/OrgasmControl.h"
#include "../include/Hardware.h"

namespace OrgasmControl {
  namespace {
    /**
     * Main orgasm detection / edging algorithm happens here.
     * This happens with a default update frequency of 50Hz.
     */
    void updateArousal() {
      // Decay stale arousal value:
      arousal *= 0.99;

      // Acquire new pressure and take average:
      pressure_value = Hardware::getPressure();
      PressureAverage.addValue(pressure_value);
      long p_avg = PressureAverage.getAverage();

      // Increment arousal:
      if (p_avg < last_value) { // falling edge of peak
        if (p_avg > peak_start) { // first tick past peak?
          if (p_avg - peak_start >= Config.sensitivity_threshold / 10) { // big peak
            arousal += p_avg - peak_start;
          }
        }
        peak_start = p_avg;
      }

      last_value = p_avg;
    }

    void updateMotorSpeed() {
      float motor_increment = (
          (float)Config.motor_max_speed /
          (50.0 * (float)Config.motor_ramp_time_s)
      );

      // Ope, orgasm incoming! Stop it!
      if (arousal > Config.sensitivity_threshold) {
        // Set the motor speed to 0, but actually set it to a negative number because cooldown delay
        motor_speed = max(-255.0f, -0.5f * (float)Config.motor_ramp_time_s * 50.0f * motor_increment);
      } else if (motor_speed < 255) {
        motor_speed += motor_increment;
      }

      // Control motor if we are not manually doing so.
      if (control_motor) {
        Hardware::setMotorSpeed(motor_speed);
      }
    }
  }

  void tick() {
    if (millis() - last_update_ms > UPDATE_FREQUENCY_MS) {
      updateArousal();
      updateMotorSpeed();
    }
  }

  /**
   * Returns a normalized motor speed from 0..255
   * @return normalized motor speed byte
   */
  byte getMotorSpeed() {
    return min((float)floor(max(motor_speed, 0.0f)), 255.0f);
  }

  float getMotorSpeedPercent() {
    return getMotorSpeed() / 255;
  }

  long getArousal() {
    return arousal;
  }

  float getArousalPercent() {
    return (float)arousal / Config.sensitivity_threshold;
  }

  long getLastPressure() {
    return pressure_value;
  }

  long getAveragePressure() {
    return PressureAverage.getAverage();
  }

  void controlMotor(bool control) {
    control_motor = control;
  }
}