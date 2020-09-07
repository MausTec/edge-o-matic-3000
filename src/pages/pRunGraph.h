#ifndef __p_RUN_GRAPH_h
#define __p_RUN_GRAPH_h

#include "../../include/Page.h"

enum RGView {
    GraphView,
    StatsView
};

class pRunGraph : public Page {
  RGView view = GraphView;

  void Enter() override {

  }

  void Render() override {
    if (view == StatsView) {
      UI.setButton(0, "CHART");
    } else {
      UI.setButton(0, "STATS");
      UI.drawChartAxes();
    }

    UI.setButton(1, "MENU");
    UI.setButton(2, "MANUAL");

    UI.drawButtons();
    UI.render();
  }

  void Loop() override {
    if (view == GraphView) {
      UI.drawChart(Config.motor_max_speed);
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
        Rerender();
        break;
      case 1:
        UI.display->dim(true);
        break;
      case 2:
        // manual <-> automatic
        break;
    }
  }

  void onEncoderChange(int diff) override {
    Serial.println("Encoder change: " + String(diff));
  }
};

#endif