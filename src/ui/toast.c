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

static struct ui_toast {
    char str[TOAST_MAX + 1];
    const char* multiline_msg;
    int8_t progress;
    uint8_t blocking;
    void (*on_close)(void*);
    void* on_close_arg;
    int start_line;
    int text_lines;
} current_toast;

void ui_toast(const char* fmt, ...) {
    ui_toast_clear();

    va_list args;
    va_start(args, fmt);
    vsnprintf(current_toast.str, TOAST_MAX, fmt, args);
    va_end(args);

    ESP_LOGD(TAG, "Toast: \"%s\", non-blocking", current_toast.str);
}

void ui_toast_multiline(const char* msg) {
    ui_toast_clear();
    current_toast.multiline_msg = msg;
}

void ui_toast_append(const char* fmt, ...) {
    char buf[TOAST_MAX + 1] = { 0 };

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, TOAST_MAX, fmt, args);
    va_end(args);

    strlcat(current_toast.str, "\n", TOAST_MAX);
    strlcat(current_toast.str, buf, TOAST_MAX);
    ui_toast_render();
    ui_send_buffer();
}

void ui_toast_set_progress(int current, int total) {
    static int8_t last_progress = -1;

    if (total == 0) {
        current_toast.progress = 0;
        return;
    }

    int pct = (current * 100) / total;

    if (pct > 100) {
        pct = 100;
    } else if (pct < 0) {
        pct = 0;
    }

    current_toast.progress = pct;

    if (current_toast.progress != last_progress) {
        ESP_LOGD(TAG, "Progress updated: %d%%", current_toast.progress);
        ui_toast_render();
        ui_send_buffer();
        last_progress = current_toast.progress;
    }
}

void ui_toast_blocking(const char* fmt, ...) {
    ui_toast_clear();

    va_list args;
    va_start(args, fmt);
    vsnprintf(current_toast.str, TOAST_MAX, fmt, args);
    va_end(args);

    current_toast.blocking = 1;
    ui_toast_render();
    ui_send_buffer();
}

void ui_toast_clear(void) {
    ESP_LOGD(TAG, "Toast clear.");
    current_toast.str[0] = '\0';
    current_toast.multiline_msg = NULL;
    current_toast.blocking = 0;
    current_toast.progress = -1;
    current_toast.on_close = NULL;
    current_toast.on_close_arg = NULL;
    current_toast.start_line = 0;
    current_toast.text_lines = 0;
}

ui_render_flag_t ui_toast_scroll(int delta) {
    size_t old = current_toast.start_line;
    current_toast.start_line += delta;

    if (current_toast.start_line > current_toast.text_lines - 4)
        current_toast.start_line = current_toast.text_lines - 4;

    if (current_toast.start_line < 0) current_toast.start_line = 0;

    return current_toast.start_line == old ? NORENDER : RENDER;
}

void ui_toast_on_close(void (*cb)(void*), void* arg) {
    ESP_LOGD(TAG, "Toast on close register!");
    current_toast.on_close = cb;
    current_toast.on_close_arg = arg;
}

void ui_toast_handle_close(void) {
    if (current_toast.on_close != NULL) {
        ESP_LOGD(TAG, "Toast closing soon!");
        current_toast.on_close(current_toast.on_close_arg);
    } else {
        ESP_LOGD(TAG, "No on close handler.");
    }
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

typedef void (*text_wrap_cb_t)(const char* start, size_t len, size_t line, void* arg);

static size_t text_wrap(size_t col, const char* text, text_wrap_cb_t cb, void* cb_arg) {
    size_t text_lines = 0;
    size_t i = 0;
    const char* line = text;
    const char* space = text;

    for (const char* ptr = text; *ptr != '\0'; ptr++) {
        i++;

        if (*ptr == '\n') {
            if (cb != NULL) cb(line, ptr - line, text_lines, cb_arg);

            i = 0;
            text_lines++;
            line = ptr + 1;
            space = line;
        } else if (i > col) {
            if (space != line) {
                if (cb != NULL) cb(line, space - line, text_lines, cb_arg);

                ptr = space;
                line = ptr;
            } else {
                if (cb != NULL) cb(line, ptr - line, text_lines, cb_arg);
            }

            i = 0;
            text_lines++;
        } else if (*ptr == ' ') {
            space = ptr + 1;
        }
    }

    if (*line != '\0' && cb != NULL) cb(line, strlen(line), text_lines, cb_arg);

    return text_lines + 1;
}

struct toast_line_cb_arg {
    u8g2_t* display;
    int x;
    int* y;
};

static void ui_toast_render_line(const char* start, size_t len, size_t i, void* arg) {
    static char line[UI_TOAST_LINE_WIDTH + 1];

    if (i - current_toast.start_line > 3 || i < current_toast.start_line) return;

    struct toast_line_cb_arg* cbarg = (struct toast_line_cb_arg*)arg;

    memcpy(line, start, len);
    line[len] = '\0';

    u8g2_DrawUTF8(cbarg->display, cbarg->x, *cbarg->y, line);
    *cbarg->y += 9;
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

    const char* str =
        current_toast.multiline_msg == NULL ? current_toast.str : current_toast.multiline_msg;

    size_t total_lines = text_wrap(UI_TOAST_LINE_WIDTH, str, NULL, NULL);
    size_t text_lines = total_lines;

    if (current_toast.text_lines == 0) {
        current_toast.text_lines = total_lines;
    }

    ESP_LOGD(TAG, "Text Lines: %d", text_lines);
    if (text_lines > 4) text_lines = 4;

    // Calculate progress bar line:
    if (current_toast.progress != -1) {
        text_lines += 1;
    }

    // TODO: This is a clusterfuck of math, and on odd lined text is off-by-one
    int start_y =
        (EOM_DISPLAY_HEIGHT / 2) - (((7 + padding) * text_lines) / 2) - (padding / 2) - margin - 1;
    if (text_lines & 1) start_y -= 1; // <-- hack for that off-by-one

    int end_y = EOM_DISPLAY_HEIGHT - start_y;
    int text_start_y = start_y + margin + padding + 1;

    ui_toast_draw_frame(display, margin, start_x, start_y, (EOM_DISPLAY_HEIGHT - (start_y * 2)));

    struct toast_line_cb_arg cb_args = {
        .display = display,
        .x = start_x + margin + padding + 1,
        .y = &text_start_y,
    };

    text_wrap(UI_TOAST_LINE_WIDTH, str, ui_toast_render_line, &cb_args);

    if (current_toast.progress >= 0) {
        int progress_x = start_x + margin + padding + 5;
        int progress_y = text_start_y + 1;
        int bar_width = EOM_DISPLAY_WIDTH - (2 * progress_x);

        u8g2_DrawFrame(display, progress_x, progress_y, bar_width, 5);
        u8g2_DrawBox(
            display, progress_x, progress_y + 1, (bar_width * current_toast.progress) / 100, 3
        );
    }

    if (total_lines > text_lines) {
        ui_draw_scrollbar(display, current_toast.start_line, total_lines, text_lines);
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