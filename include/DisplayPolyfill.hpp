#ifndef __DisplayPolyfill_hpp
#define __DisplayPolyfill_hpp

#include "u8g2.h"

enum display_color {
    SSD1306_BLACK = 0,
    SSD1306_WHITE = 1,
    COLOR_XOR = 2,
};

typedef uint8_t display_color_t;

class DisplayPolyfill {
public:
    DisplayPolyfill(u8g2_t *d) : display_ptr(d) {
        u8g2_SetFontPosTop(d);
        u8g2_SetDrawColor(d, 1);
        u8g2_SetFont(d, u8g2_font_6x10_mf);
        
    };

    // Basic Graphics
    void fillTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, display_color_t color);
    void drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, display_color_t color);
    void fillRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, display_color_t color);
    void drawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, display_color_t color);
    void drawPixel(uint8_t x, uint8_t y, display_color_t color);
    void fillScreen(display_color_t color);

    // Bitmaps
    void drawBitmap(uint8_t x, uint8_t y, const uint8_t* data, uint8_t width, uint8_t height, display_color_t color);

    // Text Stuff
    void setCursor(uint8_t x, uint8_t y);
    void setTextColor(display_color_t fg, display_color_t bg = COLOR_XOR);
    void print(const char* str);
    void print(char val);
    void print(int val);
    void setTextSize(uint8_t size);
    void printf(const char* format, ...);

    // Output
    void display();

private:
    u8g2_t *display_ptr = nullptr;

    uint8_t cursor_x = 0;
    uint8_t cursor_y = 0;
    uint8_t font_size = 1;
};

#endif