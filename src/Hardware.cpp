#include "../include/Hardware.h"

namespace Hardware {
  bool initialize() {
    initializeButtons();
    initializeEncoder();
    initializeLEDs();

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
      encoderCount = count;
    }

    analogWrite(ENCODER_RD_PIN, encoderColor.r);
    analogWrite(ENCODER_GR_PIN, encoderColor.g);
    analogWrite(ENCODER_BL_PIN, encoderColor.b);
  }

  void setLedColor(byte i, CRGB color) {
    leds[i] = color;
  }

  void ledShow() {
    FastLED.show();
  }

  namespace {
    void initializeButtons() {

      Key1.attachClick([]() {
        Serial.println("Key 1 Press!");
      });

      Key2.attachClick([]() {
        Serial.println("Key 2 Press!");
      });

      Key3.attachClick([]() {
        Serial.println("Key 3 Press!");
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
        Serial.println("Encoder Press");
      });
    }

    void initializeLEDs() {
      pinMode(LED_PIN, OUTPUT);

      FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, LED_COUNT);
      for (int i = 0; i < LED_COUNT; i++) {
        leds[i] = CRGB::Black;
      }
      FastLED.show();
    }
  }
}