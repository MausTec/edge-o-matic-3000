#ifndef __User_Interface_h
#define __User_Interface_h

#include "../config.h"

#define CHART_WIDTH SCREEN_WIDTH - 2
#define CHART_START_Y 10
#define CHART_END_Y SCREEN_HEIGHT - 11

#include <Adafruit_SSD1306.h>

class UserInterface {
public:
  UserInterface(Adafruit_SSD1306* display);

  // Common Element Drawing Functions
  void drawChartAxes();
  void drawChart(int peakLimit);

  // Chart Data
  void addChartReading(int index, int value);

  // Render Controls
  void render();

private:
  Adafruit_SSD1306* display;

  // Chart Data:
  int chartReadings[2][SCREEN_WIDTH - 2] = {{0}, {0}};
  int chartCursor[2] = {0};
};

#endif