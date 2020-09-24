#ifndef __p_RUN_GRAPH_h
#define __p_RUN_GRAPH_h

#include "../../include/Page.h"
#include "../../include/UserInterface.h"
#include "../../include/OrgasmControl.h"
#include "../../include/Hardware.h"
#include "../../include/assets.h"

enum RGView {
  GraphView,
  StatsView
};

enum RGMode {
  Manual,
  Automatic
};

class pRunGraph : public Page {
  RGView view;
  RGMode mode;

  void Enter(bool reinitialize) override {
    if (reinitialize) {
      view = StatsView;
      mode = Manual;
      OrgasmControl::controlMotor(false);
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
      UI.drawStatus("Automatic");
      UI.setButton(2, "MANUAL");
    } else {
      UI.drawStatus("Manual");
      UI.setButton(2, "AUTO");
    }
  }

  void renderChart() {
    // Update Counts
    char status[7] = "";
    byte motor = Hardware::getMotorSpeedPercent() * 100;
    byte stat_a = OrgasmControl::getArousalPercent() * 100;

    UI.display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    UI.display->setCursor(0, 10);
    sprintf(status, "M:%03d%%", motor);
    UI.display->print(status);

    UI.display->setCursor(SCREEN_WIDTH / 3 + 3, 10);
    sprintf(status, "P:%04d", OrgasmControl::getAveragePressure());
    UI.display->print(status);

    UI.display->setCursor(SCREEN_WIDTH / 3 * 2 + 7, 10);
    sprintf(status, "A:%03d%%", stat_a);
    UI.display->print(status);

    // Update Chart
    UI.drawChartAxes();
    UI.drawChart(Config.sensitivity_threshold);
  }

  void
  drawCompactBar(int x, int y, int width, int value, int maximum = 255, int lowerValue = 0, int lowerMaximum = 255) {
    // Top Text
    UI.display->setCursor(x + 1, y - 12);
    float pct1 = ((float) value / maximum);
    if (pct1 >= 1) {
      UI.display->print("Pres: MAX");
    } else {
      UI.display->printf("Pres: %02d%%", (int) floor(pct1 * 100.0f));
    }

    // Bottom Text
    UI.display->setCursor(x + 1, y + 6);
    float pct2 = ((float) lowerValue / lowerMaximum);
    if (pct2 >= 1) {
      UI.display->print("Sens: MAX");
    } else {
      UI.display->printf("Sens: %02d%%", (int) floor(pct2 * 100.0f));
    }

    // Calculate Markers
    const int bar_height = 3;
    int marker_1_x = max(x + 2, min((int) map(value, 0, maximum, x + 2, x + width - 2), x + width - 2));
    int marker_2_x = max(x + 2, min((int) map(lowerValue, 0, lowerMaximum, x + 2, x + width - 2), x + width - 2));

    // Center line + End
    UI.display->drawLine(x, y, max(x, marker_1_x - 3), y, SSD1306_WHITE); // left half
    UI.display->drawLine(min(x + width, marker_1_x + 3), y, x + width, y, SSD1306_WHITE); // right half
    UI.display->drawLine(x, y - bar_height, x, y + bar_height, SSD1306_WHITE); // left bar
    UI.display->drawLine(x + width, y - bar_height, x + width, y + bar_height, SSD1306_WHITE); // right bar

    // Markers
    UI.display->fillTriangle(marker_1_x - 2, y - bar_height, marker_1_x + 2, y - bar_height, marker_1_x, y - 1,
                             SSD1306_WHITE);
    UI.display->fillTriangle(marker_2_x - 1, y + bar_height, marker_2_x + 1, y + bar_height, marker_2_x, y + 2,
                             SSD1306_WHITE);
  }

  void drawBar(int y, char label, int value, int maximum, int limit = 0, int peak = 0) {
    int box_left = 8;
    int box_right = SCREEN_WIDTH - (6 * 3) - 3;
    int box_width = box_right - box_left;
    float pct = max(0.0f, min((float) value / (float) maximum, 1.0f));
    int pct_width = floor((float) (box_width - 2) * pct);

    // Calculate a Limit bar
    int limit_left, limit_width;

    if (limit > 0) {
      float limit_pct = max(0.0f, min((float) limit / (float) maximum, 1.0f));
      int limit_pct_width = floor((float) (box_width - 2) * (1.0f - limit_pct));
      limit_left = box_right - limit_pct_width - 1;
      limit_width = box_right - limit_left - 2;
    }

    // Calculate a Peak
    int peak_left;

    if (peak > 0) {
      float peak_pct = max(0.0f, min((float) peak / (float) maximum, 1.0f));
      peak_left = box_left + floor((float) (box_width - 2) * peak_pct);
    }

    // Render Stuff
    UI.display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    UI.display->drawRect(box_left, y, box_width, 7, SSD1306_WHITE);
    UI.display->fillRect(box_left + 1, y + 1, pct_width, 5, SSD1306_WHITE);

    // Draw Limit
    if (limit > 0) {
      for (int i = limit_left; i < limit_left + limit_width; i++) {
        for (int j = 0; j < 3; j++) {
          if ((i + j) % 2 != 0)
            continue;
          UI.display->drawPixel(i, y + 2 + j, SSD1306_WHITE);
        }
      }
    }

    // Draw Peak
    if (peak > 0) {
      UI.display->drawLine(peak_left, y, peak_left, y + 5, SSD1306_WHITE);
    }

    // Print Labels
    UI.display->setCursor(0, y);
    UI.display->print(label);
    UI.display->setCursor(box_right + 3, y);

    if (value < maximum) {
      UI.display->printf("%02d%%", (int) floor(pct * 100.0f));
    } else {
      UI.display->print("MAX");
    }
  }

  void renderStats() {
    static long arousal_peak = 0;
    static long last_peak_ms = millis();
    long arousal = OrgasmControl::getArousal();

    if (arousal > arousal_peak) {
      last_peak_ms = millis();
      arousal_peak = arousal;
    }

    if (millis() - last_peak_ms > 3000) {
      // decay peak after 3 seconds stale data
      arousal_peak *= 0.995f; // Decay Peak Value
    }

    // Motor / Arousal bars
    drawBar(10, 'M', Hardware::getMotorSpeed(), 255, mode == Automatic ? Config.motor_max_speed : 0);
    drawBar(SCREEN_HEIGHT - 18, 'A', OrgasmControl::getArousal(), 1023, Config.sensitivity_threshold, arousal_peak);

    // Pressure Icon
    int pressure_icon = map(OrgasmControl::getAveragePressure(), 0, 4095, 0, 4);
    UI.display->drawBitmap(0, 19, PLUG_ICON[pressure_icon], 24, 24, SSD1306_WHITE);

    const int horiz_split_x = (SCREEN_WIDTH / 2) + 25;

    // Pressure Bar Drawing Stuff
    const int press_x = 24 + 2;  // icon_x + icon_width + 2
    const int press_y = 19 + 12; // icon_y + (icon_height / 2)
    drawCompactBar(press_x, press_y, horiz_split_x - press_x - 9, OrgasmControl::getAveragePressure(), 4095,
                   Config.sensor_sensitivity, 255);

    // Draw a Border!
    UI.display->drawLine(horiz_split_x - 3, 19, horiz_split_x - 3, SCREEN_HEIGHT - 21, SSD1306_WHITE);

    // Orgasm Denial Counter
    UI.display->setCursor(horiz_split_x + 3, 19);
    UI.display->print("Denied");
    UI.display->setCursor(horiz_split_x + 3, 19 + 9);
    UI.display->setTextSize(2);
    UI.display->printf("%3d", OrgasmControl::getDenialCount() % 1000);
    UI.display->setTextSize(1);
  }

  void Render() override {
    if (view == StatsView) {
      renderStats();
    } else {
      renderChart();
    }

    UI.drawIcons();
    UI.drawStatus();
    UI.drawButtons();
  }

  void Loop() override {
    if (OrgasmControl::updated()) {
      if (view == GraphView) {
        // Update Chart
        UI.addChartReading(0, OrgasmControl::getAveragePressure());
        UI.addChartReading(1, OrgasmControl::getArousal());
      }

      Rerender();
    }
  }

  void onKeyPress(byte i) {
    Serial.println("Key Press: " + String(i));

    switch (i) {
      case 0:
        if (view == GraphView) {
          view = StatsView;
        } else {
          view = GraphView;
        }
        break;
      case 1:
        mode = Manual;
        Hardware::setMotorSpeed(0);
        OrgasmControl::controlMotor(false);
        break;
      case 2:
        if (mode == Automatic) {
          mode = Manual;
          OrgasmControl::controlMotor(false);
        } else {
          mode = Automatic;
          OrgasmControl::controlMotor(true);
        }
        break;
    }

    updateButtons();
    Rerender();
  }

  void onEncoderChange(int diff) override {
    const int step = 255 / 20;

    if (mode == Automatic) {
      // TODO this may go out of bounds. Also, change in steps?
      Config.sensitivity_threshold += (diff * step);
      saveConfigToSd(millis() + 2000);
    } else {
      Hardware::changeMotorSpeed(diff * step);
    }

    Rerender();
  }
};

#endif