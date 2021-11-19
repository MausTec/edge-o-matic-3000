#include "UITextInput.h"
#include "UserInterface.h"
#include <esp_log.h>

/**
 * TODO: 
 *   5) Apparently there's a heap smash when reconnecting to wifi.
 */

static const char* TAG = "UITextInput";

static const char* CS_UPPERCASE = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char* CS_LOWERCASE = "abcdefghijklmnopqrstuvwxyz";
static const char* CS_NUMERIC = "0123456789";
static const char* CS_SYMBOL = ".?!,@#$%^&*()[]{}<>-_=+/\\|;:'\" ";

// Current display driver doesn't actually support extended charsets.
// Leaving this in in case it becomes relevant in the future.
static const char* CS_CYRILLIC_UC = "ЙЦУКЕНГШЩЗХЪФЫВАПРОЛДЖЭЯЧСМИТЬБЮЁ";
static const char* CS_CYRILLIC = "йцукенгшщзхъфывапролджэячсмитьбюё";

static const char* CHARSETS[] = {
    CS_UPPERCASE,
    CS_LOWERCASE,
    CS_NUMERIC,
    CS_SYMBOL,
};

static const int CHARSETS_LEN = 4;

UITextInput::~UITextInput() {
    free(current_value);
    free(default_value);
}

void UITextInput::setValue(const char* value) {
    strncpy(current_value, value, maxlen);
    strncpy(default_value, value, maxlen);
    cursor = 0;
    this->char_edit_mode = strlen(current_value) == 0;
    select_character(value[0]);
}

void UITextInput::onConfirm(TextInputCallback cb) {
    on_confirm = cb;
}

void UITextInput::onChange(TextInputCallback cb) {
    on_change = cb;
}

void UITextInput::render() {
    UI.clear(false);

    if (prev == nullptr) {
        UI.drawStatus(title);
    } else {
        char new_title[TITLE_SIZE + 1] = "< ";
        strlcat(new_title, title, TITLE_SIZE);
        UI.drawStatus(new_title);
    }

    UI.drawIcons();
    UI.clearButtons();
    UI.display->drawLine(0, 9, SCREEN_WIDTH, 9, SSD1306_WHITE);

    if (this->char_edit_mode) {
        UI.setButton(0, "BACK", [&]() { this->exit_character_edit_mode(); });
        
        char charsetbtn[4] = "";
        strlcpy(charsetbtn, CHARSETS[this->current_charset_index], 4);
        UI.setButton(1, charsetbtn, [&]() { this->select_next_charset(); });
    } else {
        UI.setButton(0, "CANCEL", nullptr);

        if (this->cursor < this->maxlen) {
            UI.setButton(1, "INSERT", [&]() { this->insert_new_character(); });
        }
    }

    if (this->char_edit_mode || this->current_value[this->cursor] != '\0') {
        UI.setButton(2, "DELETE", [&]() { this->delete_current_character(); });
    } else {
        UI.setButton(2, "SAVE", [&]() { this->save(); });
    }

    // Render Menu

    const int context_char_len = 8;

    char left[context_char_len + 1];
    char right[context_char_len + 1];

    int left_length = min(context_char_len, cursor);
    size_t left_start_index = max(cursor - context_char_len, 0);

    strlcpy(left, current_value + left_start_index, left_length + 1);
    strlcpy(right, current_value + cursor + 1, context_char_len + 1);

    int x = (SCREEN_WIDTH / 2) - context_char_len - (strlen(left) * 6);
    int text_top = (SCREEN_HEIGHT / 2) - 3;

    UI.display->setCursor(x, text_top);
    UI.display->print(left);
    UI.display->setCursor((SCREEN_WIDTH / 2) + 9, text_top);
    UI.display->print(right);
    UI.display->setTextSize(2);

    int cur_x = (SCREEN_WIDTH / 2) - 5;
    int cur_y = text_top - 4;
    UI.display->setCursor(cur_x, cur_y);
    UI.display->print(this->get_current_char());
    UI.display->setTextSize(1);

    // grey-out current insert if null
    if (current_value[cursor] == '\0') {
        UI.drawPattern(cur_x - 1, cur_y - 1, 14, 16, 2, SSD1306_BLACK);
    }

    // draw amount left
    UI.display->setCursor(0, 11);
    UI.display->printf("%d/%d", strlen(this->current_value), this->maxlen);

    UI.drawPattern(1, text_top - 1, 18, 9, 2, SSD1306_BLACK);
    UI.drawPattern(SCREEN_WIDTH - 20, text_top - 1, 19, 9, 2, SSD1306_BLACK);

    // Up / Down
    if (char_edit_mode) {
        int tri_center = SCREEN_WIDTH / 2;
        UI.display->fillTriangle(tri_center, text_top - 10, tri_center - 2, text_top - 8, tri_center + 2, text_top - 8, SSD1306_WHITE);
        UI.display->fillTriangle(tri_center, text_top + 15, tri_center - 2, text_top + 13, tri_center + 2, text_top + 13, SSD1306_WHITE);
    } else {
        int tri_center = SCREEN_HEIGHT / 2;
        if (cursor > 0) {
            UI.display->fillTriangle(1, tri_center, 3, tri_center - 2, 3, tri_center + 2, SSD1306_WHITE);
        }
        if (cursor < strlen(current_value)) {
            int r = SCREEN_WIDTH - 2;
            UI.display->fillTriangle(r, tri_center, r-2, tri_center - 2, r-2, tri_center + 2, SSD1306_WHITE);
        }
    }

    // End Render

    UI.drawButtons();
    UI.drawToast();
    UI.render();
}

void UITextInput::tick() {
    UIMenu::tick();
}

void UITextInput::advance_character(int steps) {
    const char *charset = CHARSETS[current_charset_index];
    int charset_len = (int) strlen(charset);

    if (current_value[cursor] != '\0') {
        this->current_char_index = (current_char_index + steps) % charset_len;
        if (current_char_index < 0) current_char_index += charset_len;
    }

    char c = charset[current_char_index];
    this->current_value[this->cursor] = c;
    this->render();
}

void UITextInput::advance_cursor(int steps) {
    char current_char = current_value[cursor];

    int next_cursor = max(0, min(cursor + steps, (int) strlen(current_value)));
    char c = current_value[next_cursor];

    if (!this->char_edit_mode && current_value[cursor] == '\0' && steps > 0) {
        return;
    }
    
    // Move the null ternimator if we're advancing past the
    // end of the string.
    if (c == '\0' && steps > 0) {
        current_value[cursor + steps] = '\0';
    }

    if (c == '\0' && this->char_edit_mode) {
        select_character(current_char);
    } else {
        select_character(c);
    }

    this->cursor = next_cursor;
    this->render();
}

void UITextInput::selectNext() {
    if (char_edit_mode) {
        this->advance_character(1);
    } else {
        this->advance_cursor(1);
    }
}

void UITextInput::selectPrev() {
    if (char_edit_mode) {
        this->advance_character(-1);
    } else {
        this->advance_cursor(-1);
    }
}

void UITextInput::handleClick() {
    if (this->char_edit_mode) {
        if (this->current_value[this->cursor] == '\0') {
            this->exit_character_edit_mode();
        } else {
            this->advance_cursor(1);
        }
    } else {
        this->char_edit_mode = true;
        this->render();
        return;
    }
}

int UITextInput::getItemCount() {

}

int UITextInput::getCurrentPosition() {

}

void UITextInput::save() {
    strlcpy(this->default_value, this->current_value, this->maxlen);
    if (this->on_confirm != nullptr) {
        this->on_confirm(this->current_value, this);
    }
    UI.closeMenu();
}

UIMenu* UITextInput::close() {
    if (on_change != nullptr) {
        on_change(default_value, this);
    }
    return UIMenu::close();
}

void UITextInput::select_next_charset() {
    // We can select the trailing NULL because that's how we save.
    this->current_charset_index = (current_charset_index + 1) % CHARSETS_LEN;
    this->current_value[this->cursor] = this->get_current_char();
    this->render();
}

char UITextInput::get_current_char() {
    const char* charset = CHARSETS[current_charset_index % CHARSETS_LEN];
    if (current_char_index > strlen(charset) - 1) {
        return ' ';
    } else {
        return charset[current_char_index];
    }
}

void UITextInput::select_character(const char chr) {
    if (chr == '\0') {
        this->current_char_index = 0;
        this->current_charset_index = 0;
        return;
    }

    for (int i = 0; i < CHARSETS_LEN; i++) {
        const char* charset = CHARSETS[i];
        char* cur = strchr(charset, chr);

        if (cur != nullptr) {
            this->current_char_index = cur - charset;
            this->current_charset_index = i;
            return;
        }
    }

    this->current_char_index = 0;
    this->current_charset_index = 0;
}

void UITextInput::delete_current_character(void) {
    if (char_edit_mode) {
        cursor = cursor > 0 ? cursor - 1 : 0;
    }

    char *src = current_value + cursor + 1;
    char *dest = current_value + cursor;
    size_t len = strlen(current_value) - cursor;

    memmove(dest, src, len);

    select_character(current_value[cursor]);
    render();
}

void UITextInput::insert_new_character(void) {
    char* src = this->current_value + this->cursor;
    char* dest = this->current_value + this->cursor + 1;

    if (this->cursor > this->maxlen) {
        return;
    }

    size_t len = min(strlen(this->current_value) - this->cursor, (size_t)this->maxlen - this->cursor);

    printf("insert: src=\"%s\", dest=\"%s\", len=%d, cursor=%d\n", src, dest, len, this->cursor);
    memmove(dest, src, len);
    *(dest + len) = '\0';

    this->current_value[cursor] = 'A';
    select_character(this->current_value[cursor]);
    this->render();
}

void UITextInput::exit_character_edit_mode(void) {
    char_edit_mode = false;

    if (current_value[cursor] == '\0') {
        select_character(' ');
    } 

    render();
}