#ifndef __p_RUN_GRAPH_h
#define __p_RUN_GRAPH_h

#include "Hardware.h"
#include "orgasm_control.h"
#include "Page.h"
#include "UserInterface.h"
#include "assets.h"
#include "polyfill.h"
#include "system/websocket_handler.h"
#include <cstring>

enum RGView { GraphView, StatsView };

enum RGMode { Manual, Automatic, PostOrgasm };

class pRunGraph : public Page {
    RGView view;
    RGMode mode;

    void Enter(bool reinitialize) override {
        if (reinitialize) {
            view = StatsView;
            mode = Manual;
            orgasm_control_controlMotor(OC_MANUAL_CONTROL);
            Hardware::setPressureSensitivity(Config.sensor_sensitivity);
        }

        updateButtons();
        UI.setButton(1, "STOP");
    }

    void updateButtons() {
        if (view == StatsView) {
            UI.setButton(0, "CHART");
        } else {
            UI.setButton(0, "STATS");
        }

        if (mode == Automatic) {
            UI.drawStatus("Auto Edging");
            UI.setButton(1, "STOP");
            UI.setButton(2, "POST");
        } else if (mode == Manual) {
            UI.drawStatus("Manual");
            UI.setButton(1, "STOP");
            UI.setButton(2, "AUTO");
        } else if (mode == PostOrgasm) {
            UI.drawStatus("Edging+Orgasm");
            UI.setButton(1, "STOP");
            UI.setButton(2, "MANUAL");
        }

        if (orgasm_control_isMenuLocked()) {
            UI.setButton(1, "LOCK");
            UI.setButton(2, "LOCK");
        }
    }

    void renderChart() {
        // Update Counts
        char status[10] = "";
        uint8_t motor = Hardware::getMotorSpeedPercent() * 100;
        uint8_t stat_a = orgasm_control_getArousalPercent() * 100;

        UI.display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
        UI.display->setCursor(0, 10);
        snprintf(status, 10, "M:%03d%%", motor);
        UI.display->print(status);

        UI.display->setCursor(SCREEN_WIDTH / 3 + 3, 10);
        snprintf(status, 10, "P:%04d", orgasm_control_getAveragePressure());
        UI.display->print(status);

        UI.display->setCursor(SCREEN_WIDTH / 3 * 2 + 7, 10);
        snprintf(status, 10, "A:%03d%%", stat_a);
        UI.display->print(status);

        // Update Chart
        UI.drawChartAxes();
        UI.drawChart(Config.sensitivity_threshold);
    }

    void renderStats() {
        static long arousal_peak = 0;
        static long last_peak_ms = millis();
        long arousal = orgasm_control_getArousal();

        if (arousal > arousal_peak) {
            last_peak_ms = millis();
            arousal_peak = arousal;
        }

        if (millis() - last_peak_ms > 3000) {
            // decay peak after 3 seconds stale data
            arousal_peak *= 0.995f; // Decay Peak Value
        }

        // Motor / Arousal bars
        UI.drawBar(10, 'M', Hardware::getMotorSpeed(), 255,
                   mode == Automatic ? Config.motor_max_speed : 0);
        UI.drawBar(SCREEN_HEIGHT - 18, 'A', orgasm_control_getArousal(), 1023,
                   Config.sensitivity_threshold, arousal_peak);

        // Pressure Icon
        int pressure_icon = map(orgasm_control_getAveragePressure(), 0, 4095, 0, 4);
        UI.display->drawBitmap(0, 19, PLUG_ICON[pressure_icon], 24, 24, SSD1306_WHITE);

        const int horiz_split_x = (SCREEN_WIDTH / 2) + 25;

        // Pressure Bar Drawing Stuff
        const int press_x = 24 + 2;  // icon_x + icon_width + 2
        const int press_y = 19 + 12; // icon_y + (icon_height / 2)
        UI.drawCompactBar(press_x, press_y, horiz_split_x - press_x - 9,
                          orgasm_control_getAveragePressure(), 4095, Config.sensor_sensitivity,
                          255);

        // Draw a Border!
        UI.display->drawLine(horiz_split_x - 3, 19, horiz_split_x - 3, SCREEN_HEIGHT - 21,
                             SSD1306_WHITE);

        // Orgasm Denial Counter
        UI.display->setCursor(horiz_split_x + 3, 19);
        UI.display->print("Denied");
        UI.display->setCursor(horiz_split_x + 3, 19 + 9);
        UI.display->setTextSize(2);
        UI.display->printf("%3d", orgasm_control_getDenialCount() % 1000);
        UI.display->setTextSize(1);
    }

    void Render() override {
        if (view == StatsView) {
            renderStats();
        } else {
            renderChart();
        }

        UI.drawStatus();
        UI.drawButtons();
        UI.drawIcons();
    }

    void Loop() override {
        if (orgasm_control_updated()) {
            if (view == GraphView) {
                // Update Chart
                UI.addChartReading(0, orgasm_control_getAveragePressure());
                UI.addChartReading(1, orgasm_control_getArousal());
            }

            Rerender();
        } else {
            // UI.drawIcons();
            // UI.render();
        }
    }

    void onKeyPress(uint8_t i) {

        switch (i) {
        case 0:
            if (view == GraphView) {
                view = StatsView;
            } else {
                view = GraphView;
            }
            break;
        case 1:
            if (orgasm_control_isMenuLocked()) {
                break;
            }
            mode = Manual;
            Hardware::setMotorSpeed(0);
            orgasm_control_controlMotor(OC_MANUAL_CONTROL);
            break;
        case 2:
            if (orgasm_control_isMenuLocked()) {
                break;
            }
            if (mode == Automatic) {
                mode = PostOrgasm;
                orgasm_control_controlMotor(OC_LOCKOUT_POST_MODE);
            } else if (mode == Manual) {
                mode = Automatic;
                orgasm_control_controlMotor(OC_AUTOMAITC_CONTROL);
            } else if (mode == PostOrgasm) {
                mode = Manual;
                orgasm_control_controlMotor(OC_MANUAL_CONTROL);
            }
            break;
        }

        updateButtons();
        Rerender();
    }

    void onEncoderChange(int diff) override {
        if (orgasm_control_isMenuLocked()) {
            UI.toastNow("Access Denied", 1000);
            return;
        }

        if (mode == Automatic || mode == PostOrgasm) {
            Config.sensitivity_threshold += (diff);
            if (Config.sensitivity_threshold < 1)
                Config.sensitivity_threshold = 1;
            config_enqueue_save(millis() + 300);
        } else {
            Hardware::changeMotorSpeed(diff);
        }

        Rerender();
    }

  public:
    const char* getModeStr() {
        switch (mode) {
        case Automatic:
            return "Automatic";
        case PostOrgasm:
            return "PostOrgasm";
        case Manual:
        default:
            return "Manual";
        }
    }

    void setMode(const char* newMode) {
        if (!strcmp(newMode, "automatic")) {
            mode = Automatic;
            orgasm_control_controlMotor(OC_AUTOMAITC_CONTROL);
        } else if (!strcmp(newMode, "manual")) {
            mode = Manual;
            orgasm_control_controlMotor(OC_MANUAL_CONTROL);
        } else if (!strcmp(newMode, "postorgasm")) {
            mode = PostOrgasm;
            orgasm_control_controlMotor(OC_LOCKOUT_POST_MODE);
        } else {
            return;
        }

        updateButtons();

        // Broadcast mode:
        cJSON* root = cJSON_CreateObject();
        cJSON* mode = cJSON_AddObjectToObject(root, "mode");

        // In the modern API, primitive types live under the _ key:
        cJSON_AddStringToObject(mode, "_", getModeStr());
        // But historically, this was called "text":
        cJSON_AddStringToObject(mode, "text", getModeStr());

        websocket_broadcast(root, WS_BROADCAST_SYSTEM);
        cJSON_Delete(root);
    }

    int getMode() {
        switch (mode) {
        case Automatic:
            return 1;
        case PostOrgasm:
            return 2;
        case Manual:
        default:
            return 0;
        }
    }

    void menuUpdate() { updateButtons(); }
};

#endif
