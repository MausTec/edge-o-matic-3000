#include "Hardware.h"
#include "AccessoryDriver.h"
#include "BluetoothDriver.h"
#include "OrgasmControl.h"
#include "eom-hal.h"
#include "esp_log.h"
#include "assets.h"
#include "esp_timer.h"

#include "polyfill.h"

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

static const char* TAG = "Hardware";

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
                    UI.screenshot(NULL, 0);
                    break;

                default:
                    // noop;
                    break;
                }
            } else {
                UI.onKeyPress(button);
            }
        }

        void handle_encoder_change(int diff) {
            UI.onEncoderChange(diff);
            idle_since_ms = millis();
        }
    } // namespace

    bool initialize() {
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
        eom_hal_register_encoder_change(handle_encoder_change);

        return true;
    }

    void render_screensaver_frame() {
        static const int pos_x_max = eom_hal_get_display_width() - 24;
        static const int pos_y_max = eom_hal_get_display_height() - 24;
        static int16_t pos_x = 0;
        static int16_t pos_y = 0;
        static uint8_t direction = 0b11;
        static long last_frame_ms = 0;
        long millis = esp_timer_get_time() / 1000;

        if (millis - last_frame_ms > (1000 / 20)) {
            last_frame_ms = millis;
            UI.clear(false);
            pos_x += (direction & 1) ? 1 : -1;
            pos_y += (direction & 2) ? 1 : -1;

            if (pos_x >= pos_x_max || pos_x <= 0) {
                direction ^= 0b01;
            }

            if (pos_y >= pos_y_max || pos_y <= 0) {
                direction ^= 0b10;
            }

            int pressure_icon = map(OrgasmControl::getAveragePressure(), 0, 4095, 0, 4);
            UI.display->drawBitmap(pos_x, pos_y, PLUG_ICON[pressure_icon], 24, 24, SSD1306_WHITE);

            u8g2_SendBuffer(UI.display_ptr);
        }
    }

    void tick() {
        if ((Config.screen_dim_seconds + Config.screen_timeout_seconds) > 0 || idle || standby) {
            long idle_time_ms = millis() - idle_since_ms;

            bool do_dim =
                Config.screen_dim_seconds > 0 && idle_time_ms > Config.screen_dim_seconds * 1000;

            bool do_off = Config.screen_timeout_seconds > 0 &&
                          idle_time_ms > Config.screen_timeout_seconds * 1000;

            if (do_dim || do_off) {
                if ((!idle && do_dim) || (!standby && do_off)) {
                    // u8g2_SetPowerSave(UI.display_ptr, true);
                    u8g2_SetContrast(UI.display_ptr, 1);

                    if (do_off) {
                        UI.fadeTo();
                        UI.displayOff();
                        UI.clear(false);
                        // This calls display instead of render because here
                        // render is disabled.
                        u8g2_SendBuffer(UI.display_ptr);
                        standby = true;
                    }

                    if (do_dim && Config.enable_screensaver) {
                        ESP_LOGI(TAG, "Entering screensaver...");
                        UI.fadeTo();
                        UI.displayOff();
                        UI.clear(false);
                    }

                    idle = true;
                }

                if (idle && !standby && Config.enable_screensaver) {
                    render_screensaver_frame();
                }
            } else {
                if (idle || standby) {
                    // u8g2_SetPowerSave(UI.display_ptr, false);
                    u8g2_SetContrast(UI.display_ptr, 255);
                    UI.displayOn();
                    UI.render();
                    idle = false;
                    standby = false;
                }
            }
        }
    }

    [[deprecated("Use eom_hal_get_device_serial()")]] std::string getDeviceSerial() {
        char serial[40] = "";
        auto err = eom_hal_get_device_serial(serial, 40);
        return std::string(serial);
    }

    void setDeviceSerial(const char* serial) { ESP_LOGI(TAG, "E_DEPRECATED"); }

    void setMotorSpeed(int speed) {
        int new_speed = min(max(speed, 0), 255);
        if (new_speed == motor_speed)
            return;
        motor_speed = new_speed;

        eom_hal_set_motor_speed(motor_speed);

        BluetoothDriver::broadcastSpeed(new_speed);
        AccessoryDriver::broadcastSpeed(new_speed);
    }

    void changeMotorSpeed(int diff) {
        int new_speed = motor_speed + diff;
        setMotorSpeed(new_speed);
    }

    int getMotorSpeed() { return motor_speed; }

    float getMotorSpeedPercent() { return (float)motor_speed / 255.0; }

    [[deprecated("Use eom_hal_get_pressure_reading()")]] long getPressure() {
        return eom_hal_get_pressure_reading();
    }

    [[deprecated("Use eom_hal_set_sensor_sensitivity()")]] void
    setPressureSensitivity(uint8_t value) {
        eom_hal_set_sensor_sensitivity(value);
    }

    [[deprecated("Use eom_hal_get_sensor_sensitivity()")]] uint8_t getPressureSensitivity() {
        return eom_hal_get_sensor_sensitivity();
    }
} // namespace Hardware