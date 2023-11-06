#ifndef __Hardware_h
#define __Hardware_h

#include "config.h"

#include "UserInterface.h"

#include <FastLED.h>
#include <ESP32Encoder.h>
#include <analogWrite.h>

namespace Hardware {
  bool initialize();
  void tick();

  void setEncoderColor(CRGB color);
  void setLedColor(byte i, CRGB color = CRGB::Black);
  void ledShow();

  void setMotorSpeed(int speed);
  void changeMotorSpeed(int diff);
  int getMotorSpeed();
  float getMotorSpeedPercent();

  long getPressure();
  void setPressureSensitivity(byte value);
  /**
   * Gets the pressure sensitivity setting from the digitpot.
   * @return current digipot wiper position (1-127)
   */
  byte getPressureSensitivity();
  String getDeviceSerial();

  namespace {
    void initializeEncoder();

    void initializeButtons();

    void initializeLEDs();

    bool idle = false;
    bool standby = false;
    long idle_since_ms = 0;

    int motor_speed = 0;

#ifdef LED_PIN
    CRGB leds[LED_COUNT];
#endif
    CRGB encoderColor = CRGB::Black;

    int32_t encoderCount;
    ESP32Encoder Encoder;
  }
}

#endif
