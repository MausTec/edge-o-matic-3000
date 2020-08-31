#ifndef __Hardware_h
#define __Hardware_h

#include "arduino.h"
#include "../config.h"

#include "UserInterface.h"

#include <FastLED.h>
#include <OneButton.h>
#include <ESP32Encoder.h>
#include <analogWrite.h>

namespace Hardware {
  bool initialize();
  void tick();

  void setEncoderColor(CRGB color);
  void setLedColor(byte i, CRGB color = CRGB::Black);
  void ledShow();

  namespace {
    void initializeEncoder();

    void initializeButtons();

    void initializeLEDs();

#ifdef LED_COUNT
    CRGB leds[LED_COUNT];
#endif
    CRGB encoderColor;

    OneButton Key1(KEY_1_PIN, true, false);
    OneButton Key2(KEY_2_PIN, true, false);
    OneButton Key3(KEY_3_PIN, true, false);

    byte encoderCount;
    ESP32Encoder Encoder;
    OneButton EncoderSw(ENCODER_SW_PIN, false, false);
  }
}

#endif
