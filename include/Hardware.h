#ifndef __Hardware_h
#define __Hardware_h

#include "config.h"

#include "UserInterface.h"

struct CRGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;

  CRGB(uint16_t hex) : r((hex & 0xFF0000) >> 16), g((hex & 0x00FF00) >> 8), b((hex & 0x0000FF) >> 0) {};
  CRGB(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {};

  enum HTMLColorCode {
    Black = 0x000000,
    Red = 0xFF0000,
    Green = 0x00FF00,
    Blue = 0x0000FF,
    White = 0xFFFFFF,
  };
};

namespace Hardware {
  bool initialize();
  void tick();

  void setEncoderColor(CRGB color);
  void setLedColor(uint8_t i, CRGB color = CRGB::Black);
  void ledShow();

  void setMotorSpeed(int speed);
  void changeMotorSpeed(int diff);
  int getMotorSpeed();
  float getMotorSpeedPercent();

  long getPressure();
  void setPressureSensitivity(uint8_t value);
  /**
   * Gets the pressure sensitivity setting from the digitpot.
   * @return current digipot wiper position (1-127)
   */
  uint8_t getPressureSensitivity();
  std::string getDeviceSerial();

  namespace {
    void initializeEncoder();

    void initializeButtons();

    void initializeLEDs();

    bool idle = false;
    bool standby = false;
    long idle_since_ms = 0;

    int motor_speed = 0;
    CRGB encoderColor = CRGB::Black;
  }
}

#endif
