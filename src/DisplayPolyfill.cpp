#include "DisplayPolyfill.hpp"
#include "esp_log.h"
#include <cstdio>
#include <stddef.h>

static const char *TAG = "DisplayPolyfill";

// Basic Graphics
void DisplayPolyfill::fillTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, display_color_t color) {
    u8g2_SetDrawColor(this->display_ptr, color);
    u8g2_DrawTriangle(this->display_ptr, x1, y1, x2, y2, x3, y3);
}

void DisplayPolyfill::drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, display_color_t color) {
    u8g2_SetDrawColor(this->display_ptr, color);
    u8g2_DrawLine(this->display_ptr, x1, y1, x2, y2);
}

void DisplayPolyfill::fillRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, display_color_t color) {
    u8g2_SetDrawColor(this->display_ptr, color);
    u8g2_DrawBox(this->display_ptr, x, y, width, height);
}

void DisplayPolyfill::drawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, display_color_t color) {
    u8g2_SetDrawColor(this->display_ptr, color);
    u8g2_DrawFrame(this->display_ptr, x, y, width, height);
}

void DisplayPolyfill::drawPixel(uint8_t x, uint8_t y, display_color_t color) {
    u8g2_SetDrawColor(this->display_ptr, color);
    u8g2_DrawPixel(this->display_ptr, x, y);
}

void DisplayPolyfill::fillScreen(display_color_t color) {
    drawRect(0, 0, 128, 64, color);
}


// Bitmaps
void DisplayPolyfill::drawBitmap(uint8_t x, uint8_t y, const uint8_t* data, uint8_t width, uint8_t height, display_color_t color) {
    u8g2_SetDrawColor(this->display_ptr, color);
    u8g2_DrawBitmap(this->display_ptr, x, y, width / 8, height, data);
}


// Text Stuff
void DisplayPolyfill::setCursor(uint8_t x, uint8_t y) {
    this->cursor_x = x;
    this->cursor_y = y;

    if (this->font_size == 1 && this->cursor_y > 0) {
        // u8g2's font renders 1px lower than the other one.
        this->cursor_y -= 1;
    }

    if (this->font_size == 2) {
        // And, move the larger font down 1px to adjust.
        this->cursor_y += 1;
    }
}

void DisplayPolyfill::setTextColor(display_color_t fg, display_color_t bg) {
    u8g2_SetDrawColor(this->display_ptr, fg);
}

void DisplayPolyfill::print(const char* str) {
    u8g2_DrawStr(this->display_ptr, this->cursor_x, this->cursor_y, str);
}

void DisplayPolyfill::print(char val) {
    char str[2] = " ";
    str[0] = val;
    print(str);
}

void DisplayPolyfill::print(int val) {
    char str[12] = "";
    snprintf(str, 12, "%d", val);
    print(str);
}

void DisplayPolyfill::setTextSize(uint8_t size) {
    u8g2_SetFont(this->display_ptr, size == 1 ? u8g2_font_6x10_mf : u8g2_font_10x20_mf);
    this->font_size = size;
}

void DisplayPolyfill::printf(const char* format, ...) {
    va_list(args);
    va_start(args, format);
    size_t len = vsnprintf(NULL, 0, format, args);
    char *buf = (char*) malloc(sizeof(char)*len);
    
    if (buf != NULL) {
        vsprintf(buf, format, args);
        print(buf);
        free(buf);
    }
}


// Output
void DisplayPolyfill::display() {
    u8g2_SendBuffer(this->display_ptr);
}
