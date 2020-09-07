#include "../include/UserInterface.h"
#include "../include/Icons.h"

#include "../include/Page.h"

UserInterface::UserInterface(Adafruit_SSD1306 *display) {
  this->display = display;
}

bool UserInterface::begin() {
  if (!this->display->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    return false;
  } else {
    display->setRotation(2);
    display->cp437(true);
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  }
}

void UserInterface::drawStatus(const char *status) {
  this->display->fillRect(0, 0, SCREEN_WIDTH - 18, 10, SSD1306_BLACK);
  this->display->setCursor(0,0);
  this->display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  this->display->print(status);
}

void UserInterface::drawChartAxes() {
  // Calculate Steps
  // TODO: This still doesn't calculate correctly, but I'd like
  //       to have meaningful axis markers? Maybe???

  int chartHeight = CHART_END_Y - CHART_START_Y;
  int yInterval = chartHeight / LED_COUNT + 1;

  // Speed Meter
//  this->display->drawRect(0, 10, 4, SCREEN_HEIGHT - 10, SSD1306_WHITE);

  // Y-axis
  this->display->drawLine(CHART_START_X - 1, CHART_START_Y, CHART_START_X - 1, CHART_END_Y, SSD1306_WHITE);
  for (int i = CHART_END_Y - yInterval; i >= CHART_START_Y; i -= yInterval) {
    this->display->drawLine(CHART_START_X - 2, i, CHART_START_X - 1, i, SSD1306_WHITE);
  }

  // X-axis
  this->display->drawLine(CHART_START_X - 1, CHART_END_Y, CHART_END_X, CHART_END_Y, SSD1306_WHITE);
  for (int i = 0; i < CHART_WIDTH; i += 5) {
    this->display->drawLine(i + CHART_START_X, CHART_END_Y, i + CHART_START_X, CHART_END_Y + 1, SSD1306_WHITE);
  }
}

void UserInterface::clearButton(byte i) {
  buttons[i].show = false;
  buttons[i].text[0] = 0;
  buttons[i].fn = nullptr;
}

void UserInterface::clearButtons() {
  for (int i = 0; i < 3; i++) {
    clearButton(i);
  }
}

void UserInterface::setButton(byte i, char *text, ButtonCallback fn) {
  strcpy(buttons[i].text, text);
  if (fn != nullptr)
    buttons[i].fn = fn;
  buttons[i].show = true;
}

void UserInterface::drawButtons() {
  for (int i = 0; i < 3; i++) {
    UIButton b = buttons[i];
    if (b.show) {
      this->display->fillRect((i * BUTTON_WIDTH) + i, BUTTON_START_Y, BUTTON_WIDTH, BUTTON_HEIGHT, SSD1306_WHITE);
      this->display->setCursor((i * BUTTON_WIDTH) + i + 1, BUTTON_START_Y + 1);
      this->display->setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      this->display->print(b.text);
    }
  }
}

void UserInterface::onKeyPress(byte i) {
  ButtonCallback cb = buttons[i].fn;
  if (cb != nullptr) {
    cb();
  }

  if (Page::currentPage != nullptr) {
    Page::currentPage->onKeyPress(i);
  }
}

void UserInterface::onEncoderChange(int value) {
  if (Page::currentPage != nullptr) {
    Page::currentPage->onEncoderChange(value);
  }
}

void UserInterface::drawChart(int peakLimit = 600) {
  // Clear Chart
  this->display->fillRect(
      CHART_START_X,
      CHART_START_Y,
      CHART_WIDTH,
      CHART_HEIGHT,
      SSD1306_BLACK
  );

  int previousValue;
  int activeRow = 1;
  int seriesMax;

  // For each series:
  for (int r = 0; r < 2; r++) {
    previousValue = 0;
    seriesMax = r == 0 ? 4096 : peakLimit;

    // For each point:
    for (int i = 0; i < CHART_WIDTH; i++) {
      int valueIndex = (i + this->chartCursor[r] + 1) % CHART_WIDTH;
      int value = this->chartReadings[r][valueIndex];

      int y2 = (
          CHART_END_Y - map(
              value, 0, seriesMax, CHART_START_Y, CHART_END_Y
          ) + CHART_START_Y
      );

      int y1;
      if (i > 0) {
        y1 = (
            CHART_END_Y - map(
                previousValue, 0, seriesMax, CHART_START_Y, CHART_END_Y
            ) + CHART_START_Y
        );
      } else {
        y1 = y2;
      }

      // Constrain to window:
      y1 = min(max(CHART_START_Y, y1), CHART_END_Y);
      y2 = min(max(CHART_START_Y, y2), CHART_END_Y);

      if (activeRow == r) {
        this->display->drawLine(i + CHART_START_X, y1, i + CHART_START_X, y2, SSD1306_WHITE);
      } else if (i % 2 == 0) {
        this->display->drawPixel(i + CHART_START_X, y2, SSD1306_WHITE);
      }

      previousValue = value;
    }
  }
}

void UserInterface::setMotorSpeed(uint8_t perc) {
  int barTopY = map(perc, 0, 100, SCREEN_HEIGHT - 1, 10 + 1);
  this->display->drawRect(1, 10 + 1, 2, barTopY, SSD1306_BLACK);
  this->display->drawRect(1, barTopY, 2, SCREEN_HEIGHT, SSD1306_WHITE);
}

void UserInterface::addChartReading(int index, int value) {
  uint8_t cursor = (this->chartCursor[index] + 1) % CHART_WIDTH;
  this->chartCursor[index] = cursor;
  this->chartReadings[index][this->chartCursor[index]] = value;
}

void UserInterface::fadeTo(byte color) {
  for (byte i = 0; i < SCREEN_WIDTH; i+=4) {
    for (byte j = 0; j < SCREEN_HEIGHT; j+=4) {
      this->display->drawPixel(i, j, color);
    }
  }
  this->display->display();
  delay(50);
  for (byte i = 2; i < SCREEN_WIDTH; i+=4) {
    for (byte j = 2; j < SCREEN_HEIGHT; j+=4) {
      this->display->drawPixel(i, j, color);
    }
  }
  this->display->display();
  delay(50);
  for (byte i = 0; i < SCREEN_WIDTH; i+=4) {
    for (byte j = 2; j < SCREEN_HEIGHT; j+=4) {
      this->display->drawPixel(i, j, color);
    }
  }
  this->display->display();
  delay(100);
  for (byte i = 2; i < SCREEN_WIDTH; i+=4) {
    for (byte j = 0; j < SCREEN_HEIGHT; j+=4) {
      this->display->drawPixel(i, j, color);
    }
  }
  this->display->display();
  delay(100);
  for (byte i = 1; i < SCREEN_WIDTH; i+=2) {
    for (byte j = 1; j < SCREEN_HEIGHT; j+=2) {
      this->display->drawPixel(i, j, color);
    }
  }
  this->display->display();
  delay(125);
  for (byte i = 0; i < SCREEN_WIDTH; i+=2) {
    for (byte j = 1; j < SCREEN_HEIGHT; j+=2) {
      this->display->drawPixel(i, j, color);
    }
  }
  this->display->display();
  delay(125);
  for (byte i = 1; i < SCREEN_WIDTH; i+=2) {
    for (byte j = 0; j < SCREEN_HEIGHT; j+=2) {
      this->display->drawPixel(i, j, color);
    }
  }
  this->display->display();
}

void UserInterface::clear(bool render) {
  if (render)
    this->display->fillScreen(SSD1306_BLACK);
  else
    this->display->fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_BLACK);
}

void UserInterface::render() {
  this->display->display();
}

void UserInterface::drawWifiIcon(byte strength) {
  this->display->fillRect(SCREEN_WIDTH - 8, 0, 8, 8, SSD1306_BLACK);
  this->display->drawBitmap(SCREEN_WIDTH - 8, 0, WIFI_ICON[strength], 8, 8, SSD1306_WHITE);
}

void UserInterface::drawSdIcon(byte status) {
  this->display->fillRect(SCREEN_WIDTH - 18, 0, 8, 8, SSD1306_BLACK);
  if (status > 0)
    this->display->drawBitmap(SCREEN_WIDTH - 18, 0, SD_ICON[status-1], 8, 8, SSD1306_WHITE);
}