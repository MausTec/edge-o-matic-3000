#include "ui/menu.h"
#include "ui/toast.h"

// todo: probably we can statically allocate this whole menu but right now let's just do a dynamic
// allocator

static void on_click(const ui_menu_t* m, const ui_menu_item_t* item, const void* menu_arg) {
    const char* suffix = (const char*)item->arg;
    ui_toast("Headpats for\n%s, %s", item->label, suffix);
}

static void on_open(const ui_menu_t* m, const void* arg) {
    ui_menu_add_item(m, "Gorou", on_click, "a good doggo");
    ui_menu_add_item(m, "Itto", on_click, "big rock");
    ui_menu_add_item(m, "Zhongli", on_click, "money man");
    ui_menu_add_item(m, "Kaeya", on_click, "oh, so frosty");
    ui_menu_add_item(m, "Diluc", on_click, "juice box man");
    ui_menu_add_item(m, "Albedo", on_click, "heckin nerd");
    ui_menu_add_item(m, "Tartaglia", on_click, "murder childe");
}

DYNAMIC_MENU(MAIN_MENU, "Main Menu", on_open);
