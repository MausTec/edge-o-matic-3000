#ifndef __OrgasmControl_h
#define __OrgasmControl_h

#include <Arduino.h>
#include "config.h"
#include "RunningAverage.h"
#include "VibrationModeController.h"
#include <SD.h>

namespace OrgasmControl {
  void tick();

  // Fetch Data
  long getArousal();
  float getArousalPercent();
  byte getMotorSpeed();
  float getMotorSpeedPercent();
  long getLastPressure();
  long getAveragePressure();
  bool updated();
  int getDenialCount();

  // Set Controls
  void controlMotor(bool control = true);
  void pauseControl();
  void resumeControl();
  void resetOrgasm();

  // Recording Control
  void startRecording();
  void stopRecording();
  bool isRecording();

  // Twitch Detect (In wrong place for 60hz)
  void twitchDetect();

  // Post orgasm
  bool isMenuLocked();
  bool isPermitOrgasmReached();
  bool isPostOrgasmReached();
  bool isOrgasmDetected();
  void permitOrgasmNow(int seconds);
  void lockMenuNow(bool value);

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
    bool update_flag = false;
    bool control_motor = false;
    bool prev_control_motor = false;
    int denial_count = 0;

    // Timings - NEW
    long motor_stop_time = 0;
    long motor_start_time = 0;
    long edge_time_out = 10000;
    long random_additional_delay = 0;
    int twitch_count = 0;

    // File Writer
    long recording_start_ms = 0;
    File logfile;
    
    //  Post Orgasm Clench variables
    long clench_pressure_threshold = 4096;
    int clench_duration = 0;

    // Autoedging Time and Post-Orgasm varables
    long auto_edging_start_millis;
    long post_orgasm_start_millis;
    long post_orgasm_duration_millis;
    bool menu_is_locked = false;
    bool detected_orgasm = false;
    int post_orgasm_duration_seconds;
 

    void updateArousal();
    void updateMotorSpeed();
    void updateEdgingTime();
    VibrationModeController* getVibrationMode();
  }
}

#endif
