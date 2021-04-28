#ifndef __VibrationModeController_h
#define __VibrationModeController_h

enum class VibrationMode {
  RampStop = 1,
  Depletion = 2,
  Enhancement = 3,
  Pattern = 4,
  GlobalSync = 0
};

struct VibrationPattern {
  int motor_speed;
  float hold_ticks;
  bool ramp_to;
};

class VibrationModeController {
public:
  virtual float start() { return 0; };
  virtual float increment() { return 0; };
  virtual float stop() { return 0; };

  void tick(float motor_speed, long arousal);

protected:
  float motor_speed = 0;
  long arousal = 0;

  float rampToIncrement(int start, int target, float time_s = -1);
};

class RampStopController : public VibrationModeController {
  float start();
  float increment();
};

class EnhancementController : public VibrationModeController {
  float start();
  float increment();
  float stop();
private:
  bool stopped = false;
};

class DepletionController : public VibrationModeController {
  float start();
  float increment();

private:
  float base_speed = 0;
};

class PatternController : public VibrationModeController {
public:
  PatternController();

  float start();
  float increment();

  template <int N>
  void setPattern(VibrationPattern (&pattern)[N]);

private:
  int pattern_step = 0;
  long step_ticks = 0;

  VibrationPattern *pattern;
  int pattern_length = 0;

  VibrationPattern nextStep();
};

namespace VibrationPatterns {
  extern VibrationPattern Step[];
  extern VibrationPattern Wave[];
}

namespace VibrationControllers {
  extern RampStopController RampStop;
  extern EnhancementController Enhancement;
  extern DepletionController Depletion;
  extern PatternController Pattern;
}

#endif