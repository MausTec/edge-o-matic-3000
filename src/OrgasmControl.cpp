#include "OrgasmControl.h"
#include "Hardware.h"
#include "WiFiHelper.h"
#include "config.h"

namespace OrgasmControl {
  namespace {
    VibrationModeController* getVibrationMode() {
      switch (Config.vibration_mode) {
      case VibrationMode::Enhancement:
        return &VibrationControllers::Enhancement;

      case VibrationMode::Depletion:
        return &VibrationControllers::Depletion;

      case VibrationMode::Pattern:
        return &VibrationControllers::Pattern;

      default:
      case VibrationMode::RampStop:
        return &VibrationControllers::RampStop;
      }
    }

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
      long p_check = Config.use_average_values ? p_avg : pressure_value;

      // Increment arousal:
      if (p_check < last_value) { // falling edge of peak
        if (p_check > peak_start) { // first tick past peak?
          if (p_check - peak_start >= Config.sensitivity_threshold / 10) { // big peak
            arousal += p_check - peak_start;
          }
        }
        peak_start = p_check;
      }

      last_value = p_check;
    }

    void updateMotorSpeed() {
      if (!control_motor) return;

      VibrationModeController* controller = getVibrationMode();
      controller->tick(motor_speed, arousal);

      // Calculate timeout delay
      bool time_out_over = false;
      long on_time = millis() - motor_start_time;
      if (millis() - motor_stop_time > Config.edge_delay + random_additional_delay) {
        time_out_over = true;
      }

      // Happy ending mode: Do not deny if our good human has endured enough denials.
      bool do_not_deny = false;
      if (Config.happy_ending_mode)
      {
        // TODO: Perhaps add a ramp down to stop the vibrator after orgasm?
        do_not_deny = (denial_count >= Config.happy_ending_orgasm_num) ? true : false;
      }


      // Ope, orgasm incoming! Stop it!
      if (!time_out_over) {
        twitchDetect();
      } else if (arousal > Config.sensitivity_threshold &&
                 motor_speed > 0 &&
                 on_time > Config.minimum_on_time &&
                 do_not_deny == false) {
        // The motor_speed check above, btw, is so we only hit this once per peak.
        // Set the motor speed to 0, set stop time, and determine the new additional random time.
        motor_speed = controller->stop();
        motor_stop_time = millis();
        motor_start_time = 0;
        denial_count++;

        // If Max Additional Delay is not disabled, caculate a new delay every time the motor is stopped.
        if (Config.max_additional_delay != 0) {
          random_additional_delay = random(Config.max_additional_delay);
        }
      }

      // Start from 0
      if ((motor_speed == 0) && (motor_start_time == 0) && (time_out_over)) {
        motor_speed = controller->start();
        motor_start_time = millis();
        random_additional_delay = 0;

      // Increment or Change
      } else if (motor_speed != 0) {
        motor_speed = controller->increment();
      }

      // Control motor if we are not manually doing so.
      if (control_motor) {
        Hardware::setMotorSpeed(motor_speed);
      }
    }
  }

  void twitchDetect() {
    if (arousal > Config.sensitivity_threshold) {
      motor_stop_time = millis();
    }
  }

  /**
   * \todo Recording functions don't need to be here.
   */
  void startRecording() {
    if (logfile) {
      stopRecording();
    }

    UI.toastNow("Preapring\nrecording...", 0);

    struct tm timeinfo;
    char filename_date[16];
    if (!WiFiHelper::connected() || !getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      sprintf(filename_date, "%d", millis());
    } else {
      strftime(filename_date, 16, "%Y%m%d-%H%M%S", &timeinfo);
    }

    String logfile_name = "/log-" + String(filename_date) + ".csv";
    Serial.println("Opening logfile: " + logfile_name);
    logfile = SD.open(logfile_name, FILE_WRITE);

    if (!logfile) {
      Serial.println("Couldn't open logfile to save!" + String(logfile));
      UI.toast("Error opening\nlogfile!");
    } else {
      recording_start_ms = millis();
      logfile.println("millis,pressure,avg_pressure,arousal,motor_speed,sensitivity_threshold");
      UI.drawRecordIcon(1, 1500);
      UI.toast(String("Recording started:\n" + logfile_name).c_str());
    }
  }

  void stopRecording() {
    if (logfile) {
      UI.toastNow("Stopping...", 0);
      Serial.println("Closing logfile.");
      logfile.close();
      logfile = File();
      UI.drawRecordIcon(0);
      UI.toast("Recording stopped.");
    }
  }

  bool isRecording() {
    return (bool) logfile;
  }

  void tick() {
    long update_frequency_ms = (1.0f / Config.update_frequency_hz) * 1000.0f;

    if (millis() - last_update_ms > update_frequency_ms) {
      updateArousal();
      updateMotorSpeed();
      update_flag = true;
      last_update_ms = millis();

      // Data for logfile or classic log.
      String data =
        String(getLastPressure()) + "," +
        String(getAveragePressure()) + "," +
        String(getArousal()) + "," +
        String(Hardware::getMotorSpeed()) + "," +
        String(Config.sensitivity_threshold);

      // Write out to logfile, which includes millis:
      if (logfile) {
        logfile.println(String(last_update_ms - recording_start_ms) + "," + data);
      }

      // Write to console for classic log mode:
      if (Config.classic_serial) {
        Serial.println(data);
      }
    } else {
      update_flag = false;
    }
  }

  bool updated() {
    return update_flag;
  }

  int getDenialCount() {
    return denial_count;
  }

  /**
   * Returns a normalized motor speed from 0..255
   * @return normalized motor speed byte
   */
  byte getMotorSpeed() {
    return min((float) floor(max(motor_speed, 0.0f)), 255.0f);
  }

  float getMotorSpeedPercent() {
    return getMotorSpeed() / 255;
  }

  long getArousal() {
    return arousal;
  }

  float getArousalPercent() {
    return (float) arousal / Config.sensitivity_threshold;
  }

  long getLastPressure() {
    return pressure_value;
  }

  long getAveragePressure() {
    return PressureAverage.getAverage();
  }

  void controlMotor(bool control) {
    motor_speed = 0;
    control_motor = control;
  }

  void pauseControl() {
    prev_control_motor = control_motor;
    control_motor = false;
  }

  void resumeControl() {
    control_motor = prev_control_motor;
  }
}
