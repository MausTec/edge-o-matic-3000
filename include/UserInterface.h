#ifndef __User_Interface_h
#define __User_Interface_h

#include "../config.h"
#include "UIMenu.h"

#define BUTTON_HEIGHT 9
#define BUTTON_WIDTH  42 // (SCREEN_WIDTH / 3)
#define BUTTON_START_Y (SCREEN_HEIGHT - BUTTON_HEIGHT)

#define CHART_START_Y 20
#define CHART_END_Y (SCREEN_HEIGHT - BUTTON_HEIGHT - 2)
#define CHART_HEIGHT (CHART_END_Y - CHART_START_Y)
#define CHART_START_X 2
#define CHART_END_X (SCREEN_WIDTH)
#define CHART_WIDTH (CHART_END_X - CHART_START_X)

#define STATUS_SIZE 16
#define TOAST_WIDTH 16
#define TOAST_LINES 4

#define WIFI_ICON_IDX 0
#define SD_ICON_IDX 1
#define BT_ICON_IDX 2

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
  void tick();

  // Common Element Drawing Functions
  void drawChartAxes();
  void drawChart(int peakLimit);
  void drawStatus(const char* status = nullptr);
  void drawPattern(int start_x, int start_y, int width, int height, int pattern = 2, int color = SSD1306_WHITE);

  // Chart Data
  void addChartReading(int index, int value);
  void setMotorSpeed(uint8_t perc);

  // Render Controls
  void fadeTo(byte color = SSD1306_BLACK, bool half = false);
  void clear(bool render = true);
  void render();

  // Icons
  void drawWifiIcon(byte strength);
  void drawSdIcon(byte status);
  void drawIcons();

  // Buttons
  void clearButtons();
  void clearButton(byte i);
  void setButton(byte i, char* text, ButtonCallback fn = nullptr);
  void drawButtons();
  void onKeyPress(byte i);
  void onEncoderChange(int value);

  // Toast
  void toast(const char *message, long duration = 3000);
  void toastNow(const char *message, long duration = 3000);
  void drawToast();
  bool toastRenderPending();
  bool hasToast();

  // Menu Handling
  void openMenu(UIMenu *menu, bool save_history = true);
  UIMenu *closeMenu();
  bool isMenuOpen();

  // Debug
  void screenshot(String &buffer);
  void screenshot(void);

  Adafruit_SSD1306* display;

private:
  // Chart Data:
  int chartReadings[2][CHART_WIDTH] = {{0}, {0}};
  uint8_t chartCursor[2] = {0,0};

  // Buttons
  UIButton buttons[3];

  // Header Data
  char status[STATUS_SIZE] = {0};
  char icons[4] = {0};

  // Toast Data
  char toast_message[19*4] = "";
  long toast_expiration = 0;
  bool toast_render_pending = false;

  // Menu
  UIMenu *current_menu = nullptr;
};

extern UserInterface UI;

#endif