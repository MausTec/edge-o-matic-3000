#ifndef __ui__menu_h
#define __ui__menu_h

#ifdef __cplusplus
extern "C" {
#endif

#define UI_MENU_TITLE_MAX 64

typedef struct ui_menu {
    char title[UI_MENU_TITLE_MAX];
} ui_menu_t;

#ifdef __cplusplus
}
#endif

#endif
