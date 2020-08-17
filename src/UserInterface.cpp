#include "../include/UserInterface.h"

UserInterface::UserInterface(Adafruit_SSD1306 *display) {
  this->display = display;
}

void UserInterface::drawChartAxes() {
  int height = this->display->height();
  int width  = this->display->width();

  // Calculate Steps
  int chartHeight = CHART_END_Y - CHART_START_Y;
  int yInterval = chartHeight / LED_COUNT + 1;

  // Y-axis
  this->display->drawLine(1, CHART_START_Y, 1, CHART_END_Y, SSD1306_WHITE);
  for (int i = CHART_END_Y - yInterval; i >= CHART_START_Y; i -= yInterval) {
    this->display->drawLine(0, i, 1, i, SSD1306_WHITE);
  }

  // X-axis
  this->display->drawLine(1, CHART_END_Y, width, CHART_END_Y, SSD1306_WHITE);
  for (int i = 0; i < width - 2; i += 5) {
    this->display->drawLine(i + 2, CHART_END_Y, i + 2, CHART_END_Y + 1, SSD1306_WHITE);
  }
}

void UserInterface::drawChart(int peakLimit = 600) {
  int height = this->display->height();
  int width  = this->display->width();

  this->display->fillRect(2, CHART_START_Y, width - 2, CHART_END_Y - CHART_START_Y, SSD1306_BLACK);

  int previousValue;
  int activeRow = 1;
  int seriesMax;

  for (int r = 0; r < 2; r++) {
    previousValue = 0;
    seriesMax = r == 0 ? 4096 : peakLimit;

    for (int i = 0; i < SCREEN_WIDTH - 2; i++) {
      // TODO: SCREEN_WIDTH - 2 should be a better variable.
      int value = this->chartReadings[r][((i + this->chartCursor[r] + 1) % (SCREEN_WIDTH - 2))];

      int y2 = ((CHART_END_Y) - map(value, 0, seriesMax, CHART_START_Y, CHART_END_Y) + CHART_START_Y);

      int y1;
      if (i > 0) {
        y1 = ((CHART_END_Y) - map(previousValue, 0, seriesMax, CHART_START_Y, CHART_END_Y) + CHART_START_Y);
      } else {
        y1 = y2;
      }

      // Constrain to window:
      y1 = min(max(CHART_START_Y, y1), CHART_END_Y);
      y2 = min(max(CHART_START_Y, y2), CHART_END_Y);

      if (activeRow == r) {
        this->display->drawLine(i + 2, y1, i + 2, y2, SSD1306_WHITE);
      } else if (i % 2 == 0) {
        this->display->drawPixel(i + 2, y2, SSD1306_WHITE);
      }

      previousValue = value;
    }
  }
}

void UserInterface::addChartReading(int index, int value) {
  this->chartCursor[index] = (this->chartCursor[index] + 1) % (SCREEN_WIDTH - 2);
  this->chartReadings[index][this->chartCursor[index]] = value;
}

void UserInterface::render() {
  this->display->display();
}