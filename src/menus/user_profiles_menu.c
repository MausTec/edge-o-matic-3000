#include "ui/ui.h"

static void on_open(const ui_menu_t* m, UI_MENU_ARG_TYPE arg) {
}

DYNAMIC_MENU(USER_PROFILES_MENU, "User Profiles", on_open);