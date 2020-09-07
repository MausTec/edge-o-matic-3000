#ifndef __p_RUN_GRAPH_h
#define __p_RUN_GRAPH_h

#include "../../include/Page.h"
#include "../../include/OrgasmControl.h"
#include "../../include/Hardware.h"

enum RGView {
    GraphView,
    StatsView
};

enum RGMode {
  Manual,
  Automatic
};

class pRunGraph : public Page {
  RGView view = GraphView;
  RGMode mode = Automatic;

  void Enter() override {
    view = GraphView;
    mode = Automatic;

    UI.drawStatus("Automatic");
    UI.setButton(0, "STATS");
    UI.setButton(1, "MENU");
    UI.setButton(2, "MANUAL");
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
    if (view == GraphView) {
      // Update Counts
      char status[7] = "";
      byte motor  = Hardware::getMotorSpeedPercent() * 100;
      byte stat_a = OrgasmControl::getArousalPercent() * 100;

      UI.display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
      UI.display->setCursor(0, 10);
      sprintf(status, "M:%03d%%", motor);
      UI.display->print(status);

      UI.display->setCursor(SCREEN_WIDTH / 3, 10);
      sprintf(status, "P:%04d", OrgasmControl::getAveragePressure());
      UI.display->print(status);

      UI.display->setCursor(SCREEN_WIDTH / 3 * 2, 10);
      sprintf(status, "A:%03d%%", stat_a);
      UI.display->print(status);

      // Update Chart
      UI.drawChart(Config.sensitivity_threshold);
    }
  }

  void Render() override {
    if (view == StatsView) {
      UI.setButton(0, "CHART");
    } else {
      UI.drawChartAxes();
      renderChart();
    }

    UI.drawIcons();
    UI.drawStatus();
    UI.drawButtons();
    UI.render();
  }

  void Loop() override {
    if (view == GraphView) {
      if (OrgasmControl::updated()) {
        // Update Chart
        UI.addChartReading(0, OrgasmControl::getAveragePressure());
        UI.addChartReading(1, OrgasmControl::getArousal());
        renderChart();
        UI.render();
      }
    }
  }

  void onKeyPress(byte i) {
    Serial.println("Key Press: " + String(i));

    switch(i) {
      case 0:
        if (view == GraphView) {
          view = StatsView;
        } else {
          view = GraphView;
        }
        break;
      case 1:
        UI.display->dim(true);
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
    int step = 16;
    Serial.println("Encoder change: " + String(diff));

    if (mode == Automatic) {
      // TODO this may go out of bounds. Also, change in steps?
      Config.sensitivity_threshold += (diff * step);
    } else {
      Hardware::changeMotorSpeed(diff * step);
    }

    Rerender();
  }
};

#endif