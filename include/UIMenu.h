#ifndef __UIMenu_h
#define __UIMenu_h

#include "../config.h"
#include <functional>

class UIMenu;

typedef void(*MenuCallback)(UIMenu*);
//typedef std::function<void(UIMenu*)> MenuCallback;

typedef struct UIMenuItem {
  char *text;
  MenuCallback cb;
  UIMenuItem *next = nullptr;
  UIMenuItem *prev = nullptr;
} UIMenuItem;

class UIMenu {
public:
  UIMenu(char *t, MenuCallback = nullptr);

  void addItem(char *text, MenuCallback cb = nullptr);
  void clearItems();
  void open(UIMenu *previous = nullptr, bool save_history = true);
  UIMenu *close();
  void render();
  void initialize();
  void selectNext();
  void selectPrev();
  void handleClick();
  void tick();
  int getItemCount();
  int getCurrentPosition();

private:
  char *title;

  UIMenu *prev = nullptr;

  UIMenuItem *first_item = nullptr;
  UIMenuItem *last_item = nullptr;
  UIMenuItem *current_item = nullptr;

  MenuCallback initializer = nullptr;
  long last_menu_change = 0;
};

extern UIMenu MainMenu;

#endif