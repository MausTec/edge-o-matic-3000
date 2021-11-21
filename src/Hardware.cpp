#include "Hardware.h"
#include "OrgasmControl.h"
#include "BluetoothDriver.h"
#include "AccessoryDriver.h"
#include "eom-hal.h"

namespace Hardware {
    namespace {
        static void handle_key_press(eom_hal_button_t button, eom_hal_button_event_t event) {
            idle_since_ms = millis();

            if (event == EOM_HAL_BUTTON_HOLD) {
                switch (button) {
                case EOM_HAL_BUTTON_BACK:
                    if (OrgasmControl::isRecording()) {
                        OrgasmControl::stopRecording();
                    } else {
                        OrgasmControl::startRecording();
                    }
                    break;

                case EOM_HAL_BUTTON_MENU:
                    UI.screenshot();
                    break;
                }
            } else {
                switch (button) {
                case EOM_HAL_BUTTON_BACK:
                    UI.onKeyPress(0);
                    break;

                case EOM_HAL_BUTTON_MID:
                    UI.onKeyPress(1);
                    break;

                case EOM_HAL_BUTTON_OK:
                    UI.onKeyPress(2);
                    break;

                case EOM_HAL_BUTTON_MENU:
                    UI.onKeyPress(3);
                    break;
                }
            }
        }

        void initializeEncoder() {
            pinMode(ENCODER_RD_PIN, OUTPUT);
            pinMode(ENCODER_GR_PIN, OUTPUT);
            pinMode(ENCODER_BL_PIN, OUTPUT);

            setEncoderColor(CRGB::Black);

            ESP32Encoder::useInternalWeakPullResistors = UP;
            Encoder.attachHalfQuad(ENCODER_A_PIN, ENCODER_B_PIN);
            
            Encoder.setCount(128);
            encoderCount = 128;
        }

        void initializeLEDs() {
#ifdef LED_PIN
            pinMode(LED_PIN, OUTPUT);
            Serial.println("Setting up FastLED on pin " + String(LED_PIN));

            FastLED.addLeds<WS2812B, LED_PIN>(leds, LED_COUNT);
            for (int i = 0; i < LED_COUNT; i++) {
                leds[i] = CRGB::Green;
            }
            FastLED.show();
#endif
        }
    }


    bool initialize() {
        initializeEncoder();
        initializeLEDs();

        setPressureSensitivity(Config.sensor_sensitivity);

        // Alright so, when I made the interface for the HAL I thought it'd be great to
        // have individual registration for each button. Oh how wrong I was, friendo!
        // All my projects are now doing this, so I'm probably going to revise the interface
        // for the HAL to register one global handler, since the button and hold is passed
        // as a parameter anyway.
        eom_hal_register_button_hold(EOM_HAL_BUTTON_BACK, handle_key_press);
        eom_hal_register_button_hold(EOM_HAL_BUTTON_MENU, handle_key_press);
        eom_hal_register_button_press(EOM_HAL_BUTTON_BACK, handle_key_press);
        eom_hal_register_button_press(EOM_HAL_BUTTON_MID, handle_key_press);
        eom_hal_register_button_press(EOM_HAL_BUTTON_OK, handle_key_press);
        eom_hal_register_button_press(EOM_HAL_BUTTON_MENU, handle_key_press);

        return true;
    }

    void tick() {
        int32_t count = Encoder.getCount() / 2;
        if (count != encoderCount) {
            idle_since_ms = millis();
            UI.onEncoderChange(count - encoderCount);
            encoderCount = count;
        }

        if ((Config.screen_dim_seconds + Config.screen_timeout_seconds) > 0 || idle || standby) {
            long idle_time_ms = millis() - idle_since_ms;
            bool do_dim = Config.screen_dim_seconds > 0 && idle_time_ms > Config.screen_dim_seconds * 1000;
            bool do_off = Config.screen_timeout_seconds > 0 && idle_time_ms > Config.screen_timeout_seconds * 1000;

            if (do_dim || do_off) {
                if ((!idle && do_dim) || (!standby && do_off)) {
                    UI.display->dim(true);

                    if (do_off) {
                        UI.fadeTo();
                        UI.displayOff();
                        UI.clear(false);
                        // This calls display instead of render because here
                        // render is disabled.
                        UI.display->display();
                        standby = true;
                    }

                    idle = true;
                }
            } else {
                if (idle || standby) {
                    UI.display->dim(false);
                    UI.displayOn();
                    UI.render();
                    idle = false;
                    standby = false;
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

    [[deprecated("Use eom_hal_get_device_serial()")]]
    String getDeviceSerial() {
        char serial[40] = "";
        auto err = eom_hal_get_device_serial(serial, 40);
        return String(serial);
    }

    void setDeviceSerial(const char* serial) {
        Serial.println("E_DEPRECATED");
    }

    void setMotorSpeed(int speed) {
        int new_speed = min(max(speed, 0), 255);
        if (new_speed == motor_speed) return;
        motor_speed = new_speed;

        analogWrite(MOT_PWM_PIN, motor_speed);
        // eom_hal_set_motor_speed(motor_speed);
        
        BluetoothDriver::broadcastSpeed(new_speed);
        AccessoryDriver::broadcastSpeed(new_speed);
    }

    void changeMotorSpeed(int diff) {
        int new_speed = motor_speed + diff;
        setMotorSpeed(new_speed);
    }

    int getMotorSpeed() {
        return motor_speed;
    }

    float getMotorSpeedPercent() {
        return (float) motor_speed / 255.0;
    }

    void ledShow() {
#ifdef LED_PIN
        FastLED.show();
#endif
    }

    [[deprecated("Use eom_hal_get_pressure_reading()")]]
    long getPressure() {
        return eom_hal_get_pressure_reading();
    }

    [[deprecated("Use eom_hal_set_sensor_sensitivity()")]]
    void setPressureSensitivity(byte value) {
        eom_hal_set_sensor_sensitivity(value);
    }

    [[deprecated("Use eom_hal_get_sensor_sensitivity()")]]
    byte getPressureSensitivity() {
        return eom_hal_get_sensor_sensitivity();
    }
}