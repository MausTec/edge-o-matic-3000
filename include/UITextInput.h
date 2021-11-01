#include "UIMenu.h"

class UIInput;

typedef std::function<void(const char*, UIMenu*)> TextInputCallback;

class UITextInput : public UIMenu {
public:
    UITextInput(char* t, int len, MenuCallback cb = nullptr) : UIMenu(t, cb) {
        maxlen = len;
        current_value = (char*) malloc(len + 1);
        default_value = (char*) malloc(len + 1);

        current_value[0] = '\0';
        default_value[0] = '\0';
    };

    ~UITextInput();

    void render() override;
    void setValue(const char* value);
    void onChange(TextInputCallback cb);
    void onConfirm(TextInputCallback cb);

    void selectNext() override;
    void selectPrev() override;
    void handleClick() override;
    int getItemCount() override;
    int getCurrentPosition() override;

    UIMenu* close() override;
    void tick() override;

private:
    char *current_value = nullptr;
    char *default_value = nullptr;

    int current_char_index = 0;
    int current_charset_index = 0;

    int cursor = 0;
    int maxlen = 0;

    bool char_edit_mode = false;

    TextInputCallback on_change = nullptr;
    TextInputCallback on_confirm = nullptr;

    void set_current(const char* value);
    void get_charset_index(const char chr);
    void select_character(const char chr);
    void select_next_charset(void);
    char get_current_char(void);
    void delete_current_character(void);
    void exit_character_edit_mode(void);
    void insert_new_character(void);
    void advance_cursor(int steps);
    void advance_character(int steps);
    void save(void);
};