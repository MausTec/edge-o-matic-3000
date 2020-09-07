#ifndef __OrgasmControl_h
#define __OrgasmControl_h

#include "Arduino.h"
#include "../config.h"
#include "RunningAverage.h"

#define UPDATE_FREQUENCY_MS 20 // 50 Hz, 0.020 seconds

namespace OrgasmControl {
  void tick();

  // Fetch Data
  long getArousal();
  float getArousalPercent();
  byte getMotorSpeed();
  float getMotorSpeedPercent();
  long getLastPressure();
  long getAveragePressure();

  // Set Controls
  void controlMotor(bool control = true);

  namespace {
    long last_update_ms = 0;

    // Orgasmo Calculations
    // These can all probably be ints since we're really only using
    // 11 bits of ADC wisdom.
    RunningAverage PressureAverage;
    long last_value = 4096;
    long pressure_value = 0;
    long peak_start = 0;
    long arousal = 0;
    float motor_speed = 0;

    bool control_motor = true;

    void updateArousal();
    void updateMotorSpeed();
  }
}

#endif