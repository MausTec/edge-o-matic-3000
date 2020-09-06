#ifndef __User_Interface_h
#define __User_Interface_h

#include "../config.h"

#define CHART_START_Y 10
#define CHART_END_Y (SCREEN_HEIGHT - 11)
#define CHART_HEIGHT (CHART_END_Y - CHART_START_Y)
#define CHART_START_X 9
#define CHART_END_X (SCREEN_WIDTH)
#define CHART_WIDTH (CHART_END_X - CHART_START_X)

#define BUTTON_HEIGHT 8
#define BUTTON_WIDTH ((SCREEN_WIDTH / 3) + 1)
#define BUTTON_START_Y (SCREEN_HEIGHT - BUTTON_HEIGHT)

#include <Adafruit_SSD1306.h>

typedef void (*ButtonCallback)(void);
typedef void (*RotaryCallback)(int count);

typedef struct {
  bool show = false;
  char text[7] = "";
  ButtonCallback fn = nullptr;
} UIButton;

class UserInterface {
public:
  UserInterface(Adafruit_SSD1306* display);
  bool begin();

  // Common Element Drawing Functions
  void drawChartAxes();
  void drawChart(int peakLimit);
  void drawStatus(const char* status);

  // Chart Data
  void addChartReading(int index, int value);
  void setMotorSpeed(uint8_t perc);

  // Render Controls
  void fadeTo(byte color = SSD1306_BLACK);
  void clear();
  void render();

  // Icons
  void drawWifiIcon(byte strength);
  void drawSdIcon(byte status);

  // Buttons
  void clearButtons();
  void setButton(byte i, char* text, ButtonCallback fn);
  void drawButtons();
  void onKeyPress(byte i);

  Adafruit_SSD1306* display;

private:
  // Chart Data:
  int chartReadings[2][CHART_WIDTH] = {{0}, {0}};
  uint8_t chartCursor[2] = {0,0};

  // Buttons
  UIButton buttons[3];
};

extern UserInterface UI;

#endif