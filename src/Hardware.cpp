#include "../include/Hardware.h"
#include "../include/OrgasmControl.h"

#include <WireSlave.h>

namespace Hardware {
  bool initialize() {
    initializeButtons();
    initializeEncoder();
    initializeLEDs();

#ifndef LED_PIN
    pinMode(RJ_LED_1_PIN, OUTPUT);
    pinMode(RJ_LED_2_PIN, OUTPUT);
    digitalWrite(RJ_LED_1_PIN, LOW);
    digitalWrite(RJ_LED_2_PIN, LOW);
#endif

    Wire.begin();
    setPressureSensitivity(Config.sensor_sensitivity);

    return true;
  }

  void tick() {
#ifdef KEY_1_PIN
    Key1.tick();
    Key2.tick();
    Key3.tick();
#endif

    EncoderSw.tick();

    // Debug Encoder:
    int32_t count = Encoder.getCount();
    if (count != encoderCount) {
      idle_since_ms = millis();
      UI.onEncoderChange(count - encoderCount);
      encoderCount = count;
    }

    if (Config.screen_dim_seconds > 0) {
      if ((millis() - idle_since_ms) > Config.screen_dim_seconds * 1000) {
        if (!idle) {
          UI.display->dim(true);
          idle = true;
        }
      } else {
        if (idle) {
          UI.display->dim(false);
          idle = false;
        }
      }
    }
  }

  void setLedColor(byte i, CRGB color) {
#ifdef LED_PIN
    leds[i] = color;
#endif
  }

  void setEncoderColor(CRGB color) {
    encoderColor = color;
    analogWrite(ENCODER_RD_PIN, color.r);
    analogWrite(ENCODER_GR_PIN, color.g);
    analogWrite(ENCODER_BL_PIN, color.b);
  }

  void enableExternalBus() {
    digitalWrite(BUS_EN_PIN, LOW);
    digitalWrite(RJ_LED_1_PIN, HIGH);
    external_connected = true;
  }

  void disableExternalBus() {
    digitalWrite(BUS_EN_PIN, HIGH);
    digitalWrite(RJ_LED_1_PIN, LOW);
    external_connected = false;
  }

  void setMotorSpeed(int speed) {
    int new_speed = min(max(speed, 0), 255);
    if (new_speed == motor_speed) return;

    motor_speed = new_speed;
    analogWrite(MOT_PWM_PIN, motor_speed);

    if (external_connected) {
      Serial.println("Sending data to I2C remote...");
      Wire.beginTransmission(I2C_SLAVE_ADDR);
      Wire.write(0x10);
      Wire.write(motor_speed);
      Wire.endTransmission();
    }
  }

  void changeMotorSpeed(int diff) {
    int new_speed = motor_speed + diff;
    setMotorSpeed(new_speed);
  }

  int getMotorSpeed() {
    return motor_speed;
  }

  float getMotorSpeedPercent() {
    return (float)motor_speed / 255.0;
  }

  void ledShow() {
#ifdef LED_PIN
    FastLED.show();
#endif
  }

  long getPressure() {
    return analogRead(BUTT_PIN);
  }

  void setPressureSensitivity(byte value) {
    Wire.beginTransmission(0x2F);
    Wire.write((byte)(255 - value) / 2);
    Wire.endTransmission();
  }

  void joinI2c(byte address) {
    i2c_slave_addr = address;
    bool success = WireSlave1.begin(SDA_PIN, SCL_PIN, I2C_SLAVE_ADDR);
    if (!success) {
      Serial.println("I2C slave init failed");
      return;
    }
    WireSlave1.onReceive(handleI2c);
    Serial.println("I2C joined.");
  }

  void leaveI2c() {
    i2c_slave_addr = 0;
  }

  void handleI2c(int avail) {
    digitalWrite(RJ_LED_2_PIN, HIGH);
    char msg[32] = {0};
    int i = 0;
    while (Wire.available()) {
      msg[i++] = Wire.read();
    }
    Serial.println("Rec'd " + String(avail) + " bytes on I2C: " + String(msg));
    digitalWrite(RJ_LED_2_PIN, LOW);
  }

  namespace {
    void initializeButtons() {
      Key1.attachClick([]() {
        idle_since_ms = millis();
        UI.onKeyPress(0);
      });

      Key1.attachPress([]() {
        if (OrgasmControl::isRecording()) {
          OrgasmControl::stopRecording();
        } else {
          OrgasmControl::startRecording();
        }
      });

      Key2.attachClick([]() {
        idle_since_ms = millis();
        UI.onKeyPress(1);
      });

      Key3.attachClick([]() {
        idle_since_ms = millis();
        UI.onKeyPress(2);
      });
    }

    void initializeEncoder() {
      pinMode(ENCODER_RD_PIN, OUTPUT);
      pinMode(ENCODER_GR_PIN, OUTPUT);
      pinMode(ENCODER_BL_PIN, OUTPUT);

      setEncoderColor(CRGB::Black);

      ESP32Encoder::useInternalWeakPullResistors = UP;
      Encoder.attachSingleEdge(ENCODER_A_PIN, ENCODER_B_PIN);
      Encoder.setCount(128);
      encoderCount = 128;

      EncoderSw.attachClick([]() {
        idle_since_ms = millis();
        UI.onKeyPress(3);
      });

      // TODO: This should be EncoderSw
      EncoderSw.attachPress([]() {
        UI.screenshot();
      });
    }

    void initializeLEDs() {
#ifdef LED_PIN
      pinMode(LED_PIN, OUTPUT);

      FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, LED_COUNT);
      for (int i = 0; i < LED_COUNT; i++) {
        leds[i] = CRGB::Green;
      }
      FastLED.show();
#endif
    }
  }
}