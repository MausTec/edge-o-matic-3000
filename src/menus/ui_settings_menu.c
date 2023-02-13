#include "ui/ui.h"

static void on_open(const ui_menu_t* m, const void* arg) {
}

DYNAMIC_MENU(UI_SETTINGS_MENU, "UI Settings", on_open);