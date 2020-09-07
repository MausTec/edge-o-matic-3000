#include "../include/Hardware.h"

#include <Wire.h>

namespace Hardware {
  bool initialize() {
    initializeButtons();
    initializeEncoder();
    initializeLEDs();

    pinMode(RJ_LED_1_PIN, OUTPUT);
    pinMode(RJ_LED_2_PIN, OUTPUT);
    digitalWrite(RJ_LED_1_PIN, HIGH);
    digitalWrite(RJ_LED_2_PIN, HIGH);

    if (!Wire.begin()) {
      Serial.println("Couldn't initialize I2C");
      while (1);
    }

    return true;
  }

  void tick() {
    Key1.tick();
    Key2.tick();
    Key3.tick();

    EncoderSw.tick();

    // Debug Encoder:
    int32_t count = Encoder.getCount();
    if (count != encoderCount) {
      Serial.println("Encoder count = " + String(count));
      UI.onEncoderChange(count - encoderCount);
      encoderCount = count;
    }

    analogWrite(ENCODER_RD_PIN, encoderColor.r);
    analogWrite(ENCODER_GR_PIN, encoderColor.g);
    analogWrite(ENCODER_BL_PIN, encoderColor.b);
  }

  void setLedColor(byte i, CRGB color) {
    leds[i] = color;
  }

  void setEncoderColor(CRGB color) {
    analogWrite(ENCODER_RD_PIN, color.r);
    analogWrite(ENCODER_GR_PIN, color.g);
    analogWrite(ENCODER_BL_PIN, color.b);
  }

  void ledShow() {
#ifdef LED_PIN
    FastLED.show();
#endif
  }

  void setPressureSensitivity(byte value) {
    Wire.beginTransmission(0x2F);
    Wire.write((byte)value / 2);
    Wire.endTransmission();
  }

  namespace {
    void initializeButtons() {

      Key1.attachClick([]() {
        UI.onKeyPress(0);
      });

      Key2.attachClick([]() {
        UI.onKeyPress(1);
      });

      Key3.attachClick([]() {
        UI.onKeyPress(2);
      });
    }

    void initializeEncoder() {
      pinMode(ENCODER_RD_PIN, OUTPUT);
      pinMode(ENCODER_GR_PIN, OUTPUT);
      pinMode(ENCODER_BL_PIN, OUTPUT);

      ESP32Encoder::useInternalWeakPullResistors = UP;
      Encoder.attachSingleEdge(ENCODER_A_PIN, ENCODER_B_PIN);
      Encoder.setCount(128);
      encoderCount = 128;

      EncoderSw.attachClick([]() {
        UI.onKeyPress(3);
      });
    }

    void initializeLEDs() {
#ifdef LED_PIN
      pinMode(LED_PIN, OUTPUT);

      FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, LED_COUNT);
      for (int i = 0; i < LED_COUNT; i++) {
        leds[i] = CRGB::Black;
      }
      FastLED.show();
#endif
    }
  }
}