#include "ui/menu.h"

// todo: probably we can statically allocate this whole menu but right now let's just do a dynamic
// allocator

static void on_open(const ui_menu_t* m, void* arg) {
    ui_menu_add_item(m, "Butt Sex", NULL, NULL);
}

DYNAMIC_MENU(MAIN_MENU, "Main Menu", on_open);