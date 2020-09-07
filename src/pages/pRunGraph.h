#ifndef __p_RUN_GRAPH_h
#define __p_RUN_GRAPH_h

#include "../../include/Page.h"

class pRunGraph : public Page {
  void Enter() override {
    UI.setButton(0, "STATS");
    UI.setButton(1, "MENU");
    UI.setButton(2, "MANUAL");
  }

  void Render() override {
    UI.drawButtons();
    UI.drawChartAxes();
    UI.render();
  }

  void Loop() override {
    UI.drawChart(Config.motor_max_speed);
  }

  void onKeyPress(byte i) {
    Serial.println("Key Press: " + String(i));

    switch(i) {
      case 0:
        // stats <-> graph
        break;
      case 1:
        // menu
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