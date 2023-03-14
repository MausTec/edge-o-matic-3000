#include "ui/toast.h"
#include "eom-hal.h"
#include "esp_log.h"
#include "ui/graphics.h"
#include "ui/ui.h"
#include "util/i18n.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static const char* TAG = "ui:toast";

#define UI_TOAST_LINE_WIDTH 20

static struct {
    char str[TOAST_MAX + 1];
    const char* multiline_msg;
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

void ui_toast_multiline(const char* msg) {
    ui_toast_clear();
    current_toast.multiline_msg = msg;
    current_toast.blocking = 0;
}

void ui_toast_append(const char* fmt, ...) {
    char buf[TOAST_MAX + 1] = { 0 };

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
    ui_toast_render();
    ui_send_buffer();
}

void ui_toast_clear(void) {
    current_toast.str[0] = '\0';
    current_toast.multiline_msg = NULL;
    current_toast.blocking = 0;
}

void ui_toast_draw_frame(
    u8g2_t* d, uint8_t margin, uint8_t start_x, uint8_t start_y, uint8_t height
) {
    u8g2_SetDrawColor(d, 0);

    // Clear Border
    for (int y = 0; y < EOM_DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < EOM_DISPLAY_WIDTH; x++) {
            if ((x + y) % 2 == 0) {
                u8g2_DrawPixel(d, x, y);
            }
        }
    }

    u8g2_DrawBox(d, start_x, start_y, EOM_DISPLAY_WIDTH - (start_x * 2), height);

    u8g2_SetDrawColor(d, 1);

    u8g2_DrawFrame(
        d,
        start_x + margin,
        start_y + margin,
        EOM_DISPLAY_WIDTH - (start_x * 2) - (margin * 2),
        height - (margin * 2)
    );
}

void ui_toast_render(void) {
    if (!ui_toast_is_active()) return;

    // Line width = 18 char
    const int padding = 2;
    const int margin = 4;
    int start_x = 4;

    u8g2_t* display = eom_hal_get_display_ptr();

    u8g2_SetFont(display, UI_FONT_SMALL);
    u8g2_SetFontPosTop(display);

    int text_lines = 1;

    if (current_toast.multiline_msg == NULL) {
        for (int i = 0; i < strlen(current_toast.str); i++) {
            if (current_toast.str[i] == '\n') {
                text_lines++;
            }
        }
    } else {
        text_lines = 4;
    }

    // TODO: This is a clusterfuck of math, and on odd lined text is off-by-one
    int start_y =
        (EOM_DISPLAY_HEIGHT / 2) - (((7 + padding) * text_lines) / 2) - (padding / 2) - margin - 1;
    if (text_lines & 1) start_y -= 1; // <-- hack for that off-by-one

    int end_y = EOM_DISPLAY_HEIGHT - start_y;
    int text_start_y = start_y + margin + padding + 1;

    ui_toast_draw_frame(display, margin, start_x, start_y, (EOM_DISPLAY_HEIGHT - (start_y * 2)));

    if (current_toast.multiline_msg == NULL) {
        char* str = current_toast.str;

        while (str != NULL && *str != '\0') {
            char* tok = strchr(str, '\n');
            if (tok != NULL) *tok = '\0';

            u8g2_DrawUTF8(display, start_x + margin + padding + 1, text_start_y, str);

            if (tok != NULL) {
                *tok = '\n';
                str = tok + 1;
                text_start_y += 7 + padding;
            } else {
                break;
            }
        }
    } else {
        const char* str = current_toast.multiline_msg;
        char line[UI_TOAST_LINE_WIDTH + 1];

        size_t idx = 0;
        size_t space_idx = 0;
        uint8_t col = 0;

        do {
            if (str[idx] == ' ') {
                space_idx = idx;
            }

            if (str[idx] == '\n') {
                space_idx = idx;
                line[col] = '\0';
                col = UI_TOAST_LINE_WIDTH;
            }

            else {
                line[col] = str[idx];
                line[col + 1] = '\0';
                col++;
            }

            if (str[idx + 1] == '\0') {
                space_idx = idx;
                line[col] = '\0';
                col = UI_TOAST_LINE_WIDTH;
            }

            if (col >= UI_TOAST_LINE_WIDTH) {
                line[col - (idx - space_idx)] = '\0';
                u8g2_DrawUTF8(display, start_x + margin + padding + 1, text_start_y, line);
                text_start_y += 7 + padding;
                col = 0;
                idx = space_idx;
            }

            idx++;
        } while (str[idx] != '\0');
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
            _("Press any key...")
        );
    }
}

int ui_toast_is_active(void) {
    return current_toast.multiline_msg != NULL || current_toast.str[0] != '\0';
}

int ui_toast_is_dismissable(void) {
    return !(current_toast.blocking & 0x01);
}

const char* ui_toast_get_str(void) {
    return current_toast.multiline_msg == NULL ? current_toast.str : current_toast.multiline_msg;
}