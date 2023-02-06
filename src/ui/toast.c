#include "ui/toast.h"

#include "eom-hal.h"
#include "esp_log.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "ui/graphics.h"

static const char* TAG = "ui:toast";

static struct {
    char str[TOAST_MAX];
    uint8_t blocking;
} current_toast;

void ui_toast(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(current_toast.str, TOAST_MAX, fmt, args);
    va_end(args);

    ESP_LOGD(TAG, "Toast: \"%s\", non-blocking", current_toast.str);

    current_toast.blocking = 0;
}

void ui_toast_append(const char* fmt, ...) {
    char buf[TOAST_MAX] = { 0 };

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, TOAST_MAX, fmt, args);
    va_end(args);

    strlcat(current_toast.str, "\n", TOAST_MAX);
    strlcat(current_toast.str, buf, TOAST_MAX);
}

void ui_toast_blocking(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(current_toast.str, TOAST_MAX, fmt, args);
    va_end(args);

    current_toast.blocking = 1;
}

void ui_toast_clear(void) {
    current_toast.str[0] = '\0';
    current_toast.blocking = 0;
}

void ui_toast_render(void) {
    if (current_toast.str[0] == '\0') return;

    // Line width = 18 char
    const int padding = 2;
    const int margin = 4;
    int start_x = 4;

    u8g2_t* display = eom_hal_get_display_ptr();

    u8g2_SetFont(display, UI_FONT_SMALL);
    u8g2_SetFontPosTop(display);

    int text_lines = 1;
    for (int i = 0; i < strlen(current_toast.str); i++) {
        if (current_toast.str[i] == '\n') {
            text_lines++;
        }
    }

    // TODO: This is a clusterfuck of math, and on odd lined text is off-by-one
    int start_y =
        (EOM_DISPLAY_HEIGHT / 2) - (((7 + padding) * text_lines) / 2) - (padding / 2) - margin - 1;
    if (text_lines & 1) start_y -= 1; // <-- hack for that off-by-one

    int end_y = EOM_DISPLAY_HEIGHT - start_y;
    int text_start_y = start_y + margin + padding + 1;

    u8g2_SetDrawColor(display, 0);

    // Clear Border
    for (int y = 0; y < EOM_DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < EOM_DISPLAY_WIDTH; x++) {
            if ((x + y) % 2 == 0) {
                u8g2_DrawPixel(display, x, y);
            }
        }
    }

    u8g2_DrawBox(
        display,
        start_x,
        start_y,
        EOM_DISPLAY_WIDTH - (start_x * 2),
        EOM_DISPLAY_HEIGHT - (start_y * 2)
    );

    u8g2_SetDrawColor(display, 1);

    u8g2_DrawFrame(
        display,
        start_x + margin,
        start_y + margin,
        EOM_DISPLAY_WIDTH - (start_x * 2) - (margin * 2),
        EOM_DISPLAY_HEIGHT - (start_y * 2) - (margin * 2)
    );

    char tmp[19 * 4] = "";
    strcpy(tmp, current_toast.str);

    char* tok = strtok(tmp, "\n");
    while (tok != NULL) {
        u8g2_DrawStr(display, start_x + margin + padding + 1, text_start_y, tok);
        text_start_y += 7 + padding;
        tok = strtok(NULL, "\n");
    }

    if (!current_toast.blocking) {
        // render press-any-key message
        u8g2_SetDrawColor(display, 1);
        u8g2_DrawBox(display, 0, EOM_DISPLAY_HEIGHT - UI_BUTTON_HEIGHT, EOM_DISPLAY_WIDTH, 9);
        u8g2_SetDrawColor(display, 0);
        u8g2_SetFont(display, UI_FONT_SMALL);
        ui_draw_str_center(
            display,
            EOM_DISPLAY_WIDTH / 2,
            EOM_DISPLAY_HEIGHT - UI_BUTTON_HEIGHT,
            "Press any key..."
        );
    }
}

int ui_toast_is_active(void) {
    return current_toast.str[0] != '\0';
}

int ui_toast_is_dismissable(void) {
    return !(current_toast.blocking & 0x01);
}

const char* ui_toast_get_str(void) {
    return current_toast.str;
}