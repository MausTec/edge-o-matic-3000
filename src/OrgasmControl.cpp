#include "OrgasmControl.h"
#include "Hardware.h"
#include "WiFiHelper.h"
#include "config.h"
#include "Page.h"

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

      // detect muscle clenching.  Used in Edging+orgasm routine to detect an orgasm
      // Can also be used as an other method to compliment detecting edging 

      // raise clench threshold to pressure - 1/2 sensitivity
      if (p_check >= (clench_pressure_threshold + Config.clench_pressure_sensitivity) ) {
        clench_pressure_threshold = (p_check - (Config.clench_pressure_sensitivity/2));
      }

      // Start counting clench time if pressure over threshold
      if (p_check >= clench_pressure_threshold) {
        clench_duration += 1;
  
        // Orgasm detected
        if ( clench_duration >= Config.clench_threshold_2_orgasm && isPermitOrgasmReached()) { 
          detected_orgasm = true;
          clench_duration = 0;
        }
  
        // ajust arousal if Clench_detector in Edge is turned on
        if ( Config.clench_detector_in_edging ) {
          if ( clench_duration > (Config.clench_threshold_2_orgasm/2) ) {
            arousal += 5;     // boost arousal  because clench duration exceeded
          }
        }

        // desensitize clench threshold when clench too long. this is to stop arousal from going up
        if ( clench_duration >= Config.max_clench_duration ) { 
          clench_pressure_threshold += 10;
          clench_duration = Config.max_clench_duration;
        }

      // when not clenching lower clench time and decay clench threshold
      } else {
        clench_duration -= 5;
        if ( clench_duration <=0 ) {
          clench_duration = 0;
          // clench pressure threshold value decays over time to a min of pressure + 1/2 sensitivity
          if ( (p_check + (Config.clench_pressure_sensitivity/2)) < clench_pressure_threshold ){  
            clench_pressure_threshold *= 0.99;
          }
        }
      } // END of clench detector

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

      // Ope, orgasm incoming! Stop it!
      if (!time_out_over) {
        twitchDetect();

      } else if (arousal > Config.sensitivity_threshold && motor_speed > 0 && on_time > Config.minimum_on_time) {
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

      // Start from 0
      } else if (motor_speed == 0 && motor_start_time == 0) {
        motor_speed = controller->start();
        motor_start_time = millis();
        random_additional_delay = 0;

      // Increment or Change
      } else {
        motor_speed = controller->increment();
      }

      // Control motor if we are not manually doing so.
      if (control_motor) {
        Hardware::setMotorSpeed(motor_speed);
      }
    }
    
    void updateEdgingTime() {  // Edging+Orgasm timer
      // Make sure menu_is_locked is turned off in Manual mode
      if ( RunGraphPage.getMode() == 0 ) {  
        menu_is_locked = false;
        post_orgasm_duration_seconds = Config.post_orgasm_duration_seconds;
      }
      // keep edging start time to current time as long as system is not in Edge-Orgasm mode 2
      if ( RunGraphPage.getMode() != 2 ) {  
        auto_edging_start_millis = millis();
        post_orgasm_start_millis = 0;
      }

      // Lock Menu if turned on. and in Edging_orgasm mode
      if (Config.edge_menu_lock && !menu_is_locked) {        
        // Lock only after 2 minutes
        if ( millis() > auto_edging_start_millis + (2 * 60 * 1000)) {
          menu_is_locked = true;
          RunGraphPage.menuUpdate();
        }
      }
      
      // Pre-Orgasm loop -- Orgasm is permited
      if ( isPermitOrgasmReached() && !isPostOrgasmReached() ) {  
        Hardware::setEncoderColor(CRGB::Green);
        if (control_motor) {
          pauseControl();  // make sure orgasm is now possible
        }
        //now detect the orgasm to start post orgasm torture timer
        if (detected_orgasm) {
          post_orgasm_start_millis = millis();   // Start Post orgasm torture timer
          // Lock menu if turned on
          if (Config.post_orgasm_menu_lock && !menu_is_locked) {
            menu_is_locked = true;
            RunGraphPage.menuUpdate();
          }
          Hardware::setEncoderColor(CRGB::Red);
        }
        // raise motor speed to max speep. protect not to go higher than max
        if ( motor_speed <= (Config.motor_max_speed - 5) ) {
          motor_speed = motor_speed + 5;
          Hardware::setMotorSpeed(motor_speed);
        } else {
          motor_speed = Config.motor_max_speed;
          Hardware::setMotorSpeed(motor_speed);
        }
      } 
      
      // Post Orgasm loop
      if ( isPostOrgasmReached() ) { 
        post_orgasm_duration_millis = (post_orgasm_duration_seconds * 1000);

        // Detect if within post orgasm session
        if ( millis() < (post_orgasm_start_millis + post_orgasm_duration_millis)) { 
          motor_speed = Config.motor_max_speed;
          Hardware::setMotorSpeed(motor_speed);
        } else { // Post_orgasm timer reached
          if ( motor_speed >= 10 ) { // Ramp down motor speed to 0 
            motor_speed = motor_speed - 10;
            Hardware::setMotorSpeed(motor_speed);
          } else {
            menu_is_locked = false;
            detected_orgasm = false;
            motor_speed = 0;
            Hardware::setMotorSpeed(motor_speed);
            RunGraphPage.setMode("manual");
          }
        }
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
      logfile.println("millis,pressure,avg_pressure,arousal,motor_speed,sensitivity_threshold,clench_pressure_threshold,clench_duration");
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
      updateEdgingTime();
      updateMotorSpeed();
      update_flag = true;
      last_update_ms = millis();

      // Data for logfile or classic log.
      String data =
        String(getLastPressure()) + "," +
        String(getAveragePressure()) + "," +
        String(getArousal()) + "," +
        String(Hardware::getMotorSpeed()) + "," +
        String(Config.sensitivity_threshold) + "," +
        String(clench_pressure_threshold) + "," +
        String(clench_duration);

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

  void permitOrgasmNow(int seconds) {
    detected_orgasm = false;
    RunGraphPage.setMode("postorgasm");
    auto_edging_start_millis = millis() - (Config.auto_edging_duration_minutes * 60 * 1000);
    post_orgasm_duration_seconds = seconds;
  }

  bool isPermitOrgasmReached() {
    // Detect if edging time has passed
    if ( millis() > (auto_edging_start_millis + ( Config.auto_edging_duration_minutes * 60 * 1000 ))) {  
      return true;
    } else {
      return false;
    }
  }

  bool isPostOrgasmReached() {
    // Detect if after orgasm 
    if (post_orgasm_start_millis > 0) { 
      return true;
    } else {
      return  false;
    }
  }

  bool isMenuLocked() {
    return menu_is_locked;
  };

  void lockMenuNow(bool value) {
    menu_is_locked = value;
    RunGraphPage.menuUpdate();
  }
}
