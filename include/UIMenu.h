#ifndef __UIMenu_h
#define __UIMenu_h

#include "../config.h"

typedef void (*MenuCallback)(void);

typedef struct UIMenuItem {
  char *text;
  MenuCallback cb;
  UIMenuItem *next = nullptr;
  UIMenuItem *prev = nullptr;
} UIMenuItem;

class UIMenu {
public:
  UIMenu(char *title);

  void addItem(char *text, MenuCallback cb = nullptr);
  void open(UIMenu *previous = nullptr);
  void render();

private:
  char *title;

  UIMenu *prev = nullptr;

  UIMenuItem *first_item = nullptr;
  UIMenuItem *last_item = nullptr;
  UIMenuItem *current_item = nullptr;
};

extern UIMenu MainMenu;

#endif