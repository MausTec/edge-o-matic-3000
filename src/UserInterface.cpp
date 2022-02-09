#include "UserInterface.h"
#include "Icons.h"
#include "WebSocketHelper.h"
#include "OrgasmControl.h"

#include "Page.h"
#include "polyfill.h"
#include <cstring>
#include <math.h>
#include "esp_log.h"
#include <algorithm>

static const char *TAG = "UserInterface";

#define icon_y(idx) (SCREEN_WIDTH - (10 * (idx + 1)) + 2)

bool UserInterface::begin(u8g2_t *d) {
  this->display_ptr = d;
  this->display = new DisplayPolyfill(d);
  this->initialized = true;
  return true;
}

void UserInterface::drawStatus(const char *s) {
  if (s != nullptr) {
    WebSocketHelper::send("mode", s);
    strlcpy(status, s, STATUS_SIZE);
    int sum = 0;
    for (int i = 0; i < strlen(status); i++) {
      sum += (int)status[i];
    }

    Hardware::setEncoderColor(CRGB(
        sum % 255 >> 2,
        sum % 255 >> 1,
        sum % 255 >> 0
    ));
  }

  // actually calculate ---------------\/
  this->display->fillRect(0, 0, SCREEN_WIDTH - icon_y(3), 10, 0);
  this->display->setCursor(0,0);
  this->display->setTextColor(1, 0);
  this->display->print(status);
}

void UserInterface::drawCompactBar(int x, int y, int width, int value, int maximum, int lowerValue, int lowerMaximum) {
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
  int marker_1_x = std::max(x + 2, std::min((int) map(value, 0, maximum, x + 2, x + width - 2), x + width - 2));
  int marker_2_x = std::max(x + 2, std::min((int) map(lowerValue, 0, lowerMaximum, x + 2, x + width - 2), x + width - 2));

  // Center line + End
  UI.display->drawLine(x, y, std::max(x, marker_1_x - 3), y, 1); // left half
  UI.display->drawLine(std::min(x + width, marker_1_x + 3), y, x + width, y, 1); // right half
  UI.display->drawLine(x, y - bar_height, x, y + bar_height, 1); // left bar
  UI.display->drawLine(x + width, y - bar_height, x + width, y + bar_height, 1); // right bar

  // Markers
  UI.display->fillTriangle(marker_1_x - 2, y - bar_height, marker_1_x + 2, y - bar_height, marker_1_x, y - 1,
                           1);
  UI.display->fillTriangle(marker_2_x - 1, y + bar_height, marker_2_x + 1, y + bar_height, marker_2_x, y + 2,
                           1);
}

void UserInterface::drawBar(int y, char label, int value, int maximum, int limit, int peak) {
  int box_left = 8;
  int box_right = SCREEN_WIDTH - (6 * 3) - 3;
  int box_width = box_right - box_left;
  float pct = std::max(0.0f, std::min((float) value / (float) maximum, 1.0f));
  int pct_width = floor((float) (box_width - 2) * pct);

  // Calculate a Limit bar
  int limit_left, limit_width;

  if (limit > 0) {
    float limit_pct = std::max(0.0f, std::min((float) limit / (float) maximum, 1.0f));
    int limit_pct_width = floor((float) (box_width - 2) * (1.0f - limit_pct));
    limit_left = box_right - limit_pct_width - 1;
    limit_width = box_right - limit_left - 2;
  }

  // Calculate a Peak
  int peak_left;

  if (peak > 0) {
    float peak_pct = std::max(0.0f, std::min((float) peak / (float) maximum, 1.0f));
    peak_left = box_left + floor((float) (box_width - 2) * peak_pct);
  }

  // Render Stuff
  UI.display->setTextColor(1, 0);
  UI.display->drawRect(box_left, y, box_width, 7, 1);
  UI.display->fillRect(box_left + 1, y + 1, pct_width, 5, 1);

  // Draw Limit
  if (limit > 0) {
    for (int i = limit_left; i < limit_left + limit_width; i++) {
      for (int j = 0; j < 3; j++) {
        if ((i + j) % 2 != 0)
          continue;
        UI.display->drawPixel(i, y + 2 + j, 1);
      }
    }
  }

  // Draw Peak
  if (peak > 0) {
    UI.display->drawLine(peak_left, y, peak_left, y + 5, 1);
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

void UserInterface::drawChartAxes() {
  // Calculate Steps
  // TODO: This still doesn't calculate correctly, but I'd like
  //       to have meaningful axis markers? Maybe???

  int chartHeight = CHART_END_Y - CHART_START_Y;
  int yInterval = chartHeight / 13 + 1;

  // Speed Meter
 this->display->drawRect(0, 10, 4, SCREEN_HEIGHT - 10, 1);

  // Y-axis
  this->display->drawLine(CHART_START_X - 1, CHART_START_Y, CHART_START_X - 1, CHART_END_Y, 1);
  for (int i = CHART_END_Y - yInterval; i >= CHART_START_Y; i -= yInterval) {
    this->display->drawLine(CHART_START_X - 2, i, CHART_START_X - 1, i, 1);
  }

  // X-axis
  this->display->drawLine(CHART_START_X - 1, CHART_END_Y, CHART_END_X, CHART_END_Y, 1);
  for (int i = 0; i < CHART_WIDTH; i += 5) {
    this->display->drawLine(i + CHART_START_X, CHART_END_Y, i + CHART_START_X, CHART_END_Y + 1, 1);
  }
}

void UserInterface::clearButton(uint8_t i) {
  buttons[i].show = false;
  buttons[i].text[0] = 0;
  buttons[i].fn = nullptr;
}

void UserInterface::clearButtons() {
  for (int i = 0; i < 3; i++) {
    clearButton(i);
  }
}

void UserInterface::setButton(uint8_t i, char *text, ButtonCallback fn) {
  strcpy(buttons[i].text, text);
  if (fn != nullptr)
    buttons[i].fn = fn;
  buttons[i].show = true;
}

void UserInterface::drawPattern(int start_x, int start_y, int width, int height, int pattern, int color) {
  for (int x = start_x; x < start_x + width; x++) {
    for (int y = start_y; y < start_y + height; y++) {
      if (!((x+y)%pattern)) {
        this->display->drawPixel(x, y, color);
      }
    }
  }
}

void UserInterface::drawButtons() {
  this->display->drawLine(0, BUTTON_START_Y-1, SCREEN_WIDTH, BUTTON_START_Y-1, 0);

  for (int i = 0; i < 3; i++) {
    UIButton b = buttons[i];
    int offset_left = 1;
    int text_width = (strlen(b.text) * 6) - 1;
    int button_start_l = (i * BUTTON_WIDTH) + i;

    if (i == 1) {
      offset_left = (BUTTON_WIDTH - text_width) / 2;
    } else if (i == 2) {
      offset_left = BUTTON_WIDTH - text_width - 1;
    }

    if (b.show) {
      this->display->fillRect(button_start_l, BUTTON_START_Y, BUTTON_WIDTH, BUTTON_HEIGHT, 1);
      this->display->setTextColor(0, 1);
      this->display->setCursor(button_start_l + offset_left, BUTTON_START_Y + 1);
      this->display->print(b.text);
    } else {
      this->drawPattern(button_start_l, BUTTON_START_Y, BUTTON_WIDTH, BUTTON_HEIGHT, 3, 1);
    }
  }
}

void UserInterface::onKeyPress(uint8_t i) {
  if (hasToast()) {
    if (toast_allow_clear) {
      UI.toast("", 0);
    }
    return;
  }

  // don't lock Chart button (i == 0)  
  if (i != 0 && OrgasmControl::isMenuLocked()){
    UI.toastNow("Access Denied", 1000);
    return;
  }

  // FIXME: This segfaults in the menu context on btn3 (encoder)
  ButtonCallback cb = buttons[i].fn;
  if (cb != nullptr) {
    return cb();
  }

  if (UI.isMenuOpen()) {
    if (i == 0) {
      UI.closeMenu();
    } else {
      current_menu->handleClick();
    }

    return;
  } else if (i == 3) {
    UI.openMenu(&MainMenu);
    return;
  }

  if (Page::currentPage != nullptr) {
    Page::currentPage->onKeyPress(i);
  }
}

void UserInterface::onEncoderChange(int value) {
  if (UI.isMenuOpen()) {
    // Todo we can pass diff here to provide a better fast scrolly experience.
    if (value < 0) {
      current_menu->selectPrev();
    } else {
      current_menu->selectNext();
    }
    return;
  }

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
      0
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
      y1 = std::min(std::max(CHART_START_Y, y1), CHART_END_Y);
      y2 = std::min(std::max(CHART_START_Y, y2), CHART_END_Y);

      if (activeRow == r) {
        this->display->drawLine(i + CHART_START_X, y1, i + CHART_START_X, y2, 1);
      } else if (i % 2 == 0) {
        this->display->drawPixel(i + CHART_START_X, y2, 1);
      }

      previousValue = value;
    }
  }
}

void UserInterface::setMotorSpeed(uint8_t perc) {
  int barTopY = map(perc, 0, 100, SCREEN_HEIGHT - 1, 10 + 1);
  this->display->drawRect(1, 10 + 1, 2, barTopY, 0);
  this->display->drawRect(1, barTopY, 2, SCREEN_HEIGHT, 1);
}

void UserInterface::addChartReading(int index, int value) {
  uint8_t cursor = (this->chartCursor[index] + 1) % CHART_WIDTH;
  this->chartCursor[index] = cursor;
  this->chartReadings[index][this->chartCursor[index]] = value;
}

void UserInterface::fadeTo(uint8_t color, bool half) {
  // TODO: This should fade a,b,i=[2,2,4],[0,2,4],[2,0,4],[1,1,2],[0,1,2],[1,0,2]
  for (uint8_t a = 0; a < 2; a++) {
    for (uint8_t b = 0; b < 2; b++) {
      uint8_t increment = (a == 1 || b == 1) ? 2 : 4;
      for (uint8_t i = a; i < SCREEN_WIDTH; i += increment) {
        for (uint8_t j = b; j < SCREEN_HEIGHT; j += increment) {
          this->display->drawPixel(i, j, color);
        }
      }

      this->display->display();
      if (!half)
        delay(200 / increment);
    }
  }
}

void UserInterface::clear(bool render) {
  if (render)
    this->display->fillScreen(0);
  else
    this->display->fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
}

void UserInterface::render() {
  if (display_on) {
    this->display->display();
  }
}

void UserInterface::drawIcon(uint8_t icon_idx, uint8_t icon_graphic[][8], uint8_t status, long flash_ms) {
  UIIcon *icon = &icons[icon_idx];
  uint8_t icon_frame_idx;

  if (status != 255) {
    icon->status = status;
    icon->flash_delay = flash_ms;
    icon->show = 1;
    icon->last_flash = millis();
    icon_frame_idx = status;
  } else {
    icon_frame_idx = icon->status;
    if (icon->flash_delay != 0 && ((millis() - icon->last_flash) > icon->flash_delay)) {
      icon->last_flash = millis();
      icon->show = !icon->show;
    }
  }

  if (this->initialized) {
    this->display->fillRect(icon_y(icon_idx), 0, 8, 8, 0);
    if (icon_frame_idx > 0 && icon->show)
      this->display->drawBitmap(icon_y(icon_idx), 0, icon_graphic[icon_frame_idx - 1], 8, 8, 1);
  }
}

void UserInterface::drawWifiIcon(uint8_t status, long flash_ms) {
  drawIcon(WIFI_ICON_IDX, WIFI_ICON, status, flash_ms);
}

void UserInterface::drawSdIcon(uint8_t status, long flash_ms) {
  drawIcon(SD_ICON_IDX, SD_ICON, status, flash_ms);
}

void UserInterface::drawRecordIcon(uint8_t status, long flash_ms) {
  drawIcon(RECORD_ICON_IDX, RECORD_ICON, status, flash_ms);
}

void UserInterface::drawUpdateIcon(uint8_t status, long flash_ms) {
  drawIcon(UPDATE_ICON_IDX, UPDATE_ICON, status, flash_ms);
}

/**
 * Redraw icons from memory.
 */
void UserInterface::drawIcons() {
  drawWifiIcon();
  drawSdIcon();
  drawRecordIcon();
  drawUpdateIcon();
}

void UserInterface::screenshot(std::string &out_data) {
  int buffer_size = SCREEN_WIDTH * ((SCREEN_HEIGHT + 7) / 8);
  // uint8_t *buffer = display->getBuffer();
  // String data = "";

  for (int i = 0; i < buffer_size; i++) {
    // data += String((buffer[i] & 0xF0) >> 4, HEX);
    // data += String(buffer[i] & 0x0F, HEX);
  }

  // out_data = data;
  out_data = "";
}

void UserInterface::screenshot() {
  // String buffer;
  // screenshot(buffer);
  // Serial.println(buffer);
  // toast("Screenshot Saved", 3000);
}

void UserInterface::drawToast() {
  toast_render_pending = false;

  // Can C++ just let me do toast_message == ""
  // because that'd be prettier.
  if (toast_message[0] == '\0')
    return;

  if (toast_expiration != 0 && millis() > toast_expiration) {
    toast_message[0] = '\0';
    toast_render_pending = true;
    return;
  }

  // Line width = 18 char
  const int padding = 2;
  const int margin = 4;
  int start_x = 4;

  int text_lines = 1;
  for (int i = 0; i < strlen(toast_message); i++) {
    if (toast_message[i] == '\n') {
      text_lines++;
    }
  }

  // TODO: This is a clusterfuck of math, and on odd lined text is off-by-one
  int start_y = (SCREEN_HEIGHT/2) - (((7+padding)*text_lines)/2) - (padding/2) - margin - 1;
  if (text_lines & 1) start_y -= 1; // <-- hack for that off-by-one

  int end_y = SCREEN_HEIGHT - start_y;
  int text_start_y = start_y + margin + padding + 1;

  // Clear Border
  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      if((x+y) % 2 == 0) {
        display->drawPixel(x, y, 0);
      }
    }
  }

  display->fillRect(start_x, start_y, SCREEN_WIDTH - (start_x * 2), SCREEN_HEIGHT - (start_y * 2), 0);
  display->drawRect(start_x + margin, start_y + margin, SCREEN_WIDTH - (start_x * 2) - (margin * 2), SCREEN_HEIGHT - (start_y * 2) - (margin * 2), 1);
  display->setTextColor(1, 0);

  char tmp[19*4] = "";
  strcpy(tmp, toast_message);

  char *tok = strtok(tmp, "\n");
  while (tok != NULL) {
    display->setCursor(start_x + margin + padding + 1, text_start_y);
    display->print(tok);
    text_start_y += 7 + padding;
    tok = strtok(NULL, "\n");
  }

  if (toast_allow_clear) {
    // render press-any-key message
    display->fillRect(0, SCREEN_HEIGHT - 9, SCREEN_WIDTH, 9, 1);
    display->setCursor(1, SCREEN_HEIGHT - 8);
    display->setTextSize(1);
    display->setTextColor(0);
    display->print("Press any key...");
  }
}

void UserInterface::toast(const char *message, long duration, bool allow_clear) {
  strlcpy(toast_message, message, 19*4);
  toast_expiration = duration > 0 ? millis() + duration : 0;
  toast_render_pending = true;
  toast_allow_clear = allow_clear;
}

void UserInterface::toast(std::string message, long duration, bool allow_clear) {
  toast(message.c_str(), duration, allow_clear);
}

void UserInterface::toastNow(const char *message, long duration, bool allow_clear) {
  toast(message, duration, allow_clear);
  drawToast();
  render();
}

void UserInterface::toastNow(std::string message, long duration, bool allow_clear) {
  toastNow(message.c_str(), duration, allow_clear);
}

void UserInterface::toastProgress(const char *message, float progress) {
  static int lastPerc = 0;
  int perc = std::max(0.0, std::min(progress * 100.0, 100.0));

  // Don't redraw if the percent didn't actually change.
  if (perc == lastPerc) {
    return;
  } else {
    lastPerc = perc;
  }

  char progressBar[TOAST_WIDTH + 1] = "";
  char progressFmt[TOAST_WIDTH + 1] = "";
  char percFill[TOAST_WIDTH - 6] = "";
  char finalToast[TOAST_WIDTH * TOAST_LINES + 5] = "";

  int fillBars = floor(((float)perc / 100) * sizeof(percFill));
  for (int i = 0; i < fillBars; i++) {
    strcat(percFill, "#");
  }

  sprintf(progressFmt, "[%%-%ds] %%3d%%%%", TOAST_WIDTH - 7);
  sprintf(progressBar, progressFmt, percFill, perc);

  strcpy(finalToast, message);
  strcat(finalToast, "\n");
  strcat(finalToast, progressBar);
  ESP_LOGD(TAG, "%s", progressBar);

  toastNow(finalToast, 0, false);
}

void UserInterface::toastProgress(std::string message, float progress) {
  toastProgress(message.c_str(), progress);
}

bool UserInterface::isMenuOpen() {
  return current_menu != nullptr;
}

UIMenu *UserInterface::closeMenu() {
  if (current_menu == nullptr)
    return nullptr;

  UIMenu *prev = current_menu->close();
  openMenu(prev, false);
  return prev;
}

void UserInterface::openMenu(UIMenu *menu, bool save_history, bool reenter, void *arg) {
  if (menu != nullptr) {
    menu->open(current_menu, save_history, arg);
  }

  current_menu = menu;

  if (current_menu == nullptr && reenter) {
    Page::Reenter();
  }
}

bool UserInterface::toastRenderPending() {
  bool trp = toast_render_pending || (toast_message[0] != '\0' && millis() > toast_expiration);
  return trp;
}

void UserInterface::tick() {
  if (current_menu != nullptr) {
    current_menu->tick();
  }
}

bool UserInterface::hasToast() {
  bool has_toast = toast_message[0] != '\0' && (toast_expiration == 0 || millis() < toast_expiration);
  return has_toast;
}