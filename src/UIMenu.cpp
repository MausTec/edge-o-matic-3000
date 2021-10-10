#include "UIMenu.h"
#include "UserInterface.h"
#include "Page.h"

#include <esp32-hal-log.h>

UIMenu::UIMenu(const char *t, MenuCallback fn) {
  strlcpy(title, t, TITLE_SIZE);
  initializer = fn;
}

void UIMenu::addItem(const char *text, MenuCallback cb) {
  UIMenuItem *item = new UIMenuItem();
  strncpy(item->text, text, 20);
  item->next = nullptr;
  item->cb = cb;
  item->pcb = nullptr;
  item->prev = last_item;

  if (last_item != nullptr) {
    last_item->next = item;
  }

  last_item = item;

  if (first_item == nullptr) {
    first_item = item;
  }
}

void UIMenu::addItem(const char *text, IParameterizedMenuCallback cb, int arg) {
  UIMenuItem *item = new UIMenuItem();
  strncpy(item->text, text, 20);
  item->next = nullptr;
  item->cb = nullptr;
  item->pcb = nullptr;
  item->ipcb = cb;
  item->prev = last_item;
  item->iarg = arg;

  if (last_item != nullptr) {
    last_item->next = item;
  }

  last_item = item;

  if (first_item == nullptr) {
    first_item = item;
  }
}

void UIMenu::addItem(const char *text, ParameterizedMenuCallback pcb, void *arg) {
  UIMenuItem *item = new UIMenuItem();
  strncpy(item->text, text, 20);
  item->next = nullptr;
  item->cb = nullptr;
  item->pcb = pcb;
  item->prev = last_item;
  item->arg = arg;

  if (last_item != nullptr) {
    last_item->next = item;
  }

  last_item = item;

  if (first_item == nullptr) {
    first_item = item;
  }
}

void UIMenu::removeItem(size_t index) {
  UIMenuItem *ptr = this->first_item;

  for (int i = 0; i < index && ptr != nullptr; i++) {
    ptr = ptr->next;
  }

  if (ptr == nullptr) {
    return;
  }

  if (first_item == ptr) {
    this->first_item = ptr->next;
  }

  if (last_item == ptr) {
    this->last_item = nullptr;
  }

  if (ptr->prev != nullptr) {
    ptr->prev->next = ptr->next;
  }

  if (ptr->next != nullptr) {
    ptr->next->prev = ptr->prev;
  }

  delete ptr;
}

void UIMenu::addItemAt(size_t index, const char *text, MenuCallback cb) {
  UIMenuItem *ptr = this->first_item;

  for (int i = 0; i < index && ptr != nullptr; i++) {
    ptr = ptr->next;
  }

  // Last or First item:
  if (ptr == nullptr) {
    addItem(text, cb);
    return;
  }

  UIMenuItem *item = new UIMenuItem();
  strncpy(item->text, text, 20);
  item->next = nullptr;
  item->cb = cb;
  item->pcb = nullptr;
  item->prev = ptr->prev;
  item->next = ptr;

  if (ptr == this->first_item) {
    this->first_item = item;
  }

  if (ptr->prev != nullptr) {
    ptr->prev->next = item;
  }
  ptr->prev = item;
}

void UIMenu::addItem(UIMenu *submenu, void *arg) {
  addItem(submenu->title, [=](UIMenu*, void *argv) {
    UI.openMenu(submenu, true, true, argv);
  }, arg);
}

void UIMenu::addItem(std::string text, UIMenu *submenu, void *arg) {
  addItem(text.c_str(), [=](UIMenu*, void *argv) {
    log_i("Item added with argv=%x", argv);
    UI.openMenu(submenu, true, true, argv);
  }, arg);
}

void UIMenu::addItem(const char *text, Page *page) {
  addItem(text, [=](UIMenu*) {
    Page::Go(page);
  });
}

void UIMenu::render() {
  UI.clear(false);

  if (prev == nullptr) {
    UI.drawStatus(title);
  } else {
    char new_title[TITLE_SIZE + 1] = "< ";
    strlcat(new_title, title, TITLE_SIZE);
    UI.drawStatus(new_title);
  }

  UI.drawIcons();
  UI.display->drawLine(0, 9, SCREEN_WIDTH, 9, SSD1306_WHITE);

  // Step back 2 items or to start
  UIMenuItem *item = current_item;
  for (int i = 0; i < 2; i++) {
    if (item == nullptr || item->prev == nullptr) {
      break;
    }
    item = item->prev;
  }

  // Render the next 5 items
  if (item != nullptr) {
    for (int count = 0; count < 5; count++) {
      int menu_item_y = 10 + (10 * count);
      UI.display->setCursor(2, menu_item_y + 1);
      if (current_item == item) {
        UI.display->fillRect(0, menu_item_y, SCREEN_WIDTH, 9, SSD1306_WHITE);
        UI.display->setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      } else {
        UI.display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
      }

      UI.display->print(item->text);

      item = item->next;
      if (item == nullptr) {
        break;
      }
    }

    // Count items and get position
    int menu_item_count = getItemCount();
    int current_item_position = getCurrentPosition();

    // Render Scrollbar
    if (menu_item_count > 3) {
      const int scroll_track_start_y = 11;
      const int scroll_track_end_y = SCREEN_HEIGHT - 10;
      const int scroll_track_height = scroll_track_end_y - scroll_track_start_y;
      int scroll_height = max((int) floor((float) scroll_track_height / ((float) menu_item_count / 4)), 4);
      int scroll_start_y;
      if (menu_item_count >= 1) {
        scroll_start_y = floor((float) (scroll_track_height - scroll_height) *
                               ((float) (current_item_position - 1) / (menu_item_count - 1)));
      } else {
        scroll_start_y = 0;
      }

      UI.display->fillRect(SCREEN_WIDTH - 3, scroll_track_start_y - 1, 3, scroll_track_height + 1, SSD1306_BLACK);
      UI.display->fillRect(SCREEN_WIDTH - 2, scroll_track_start_y + scroll_start_y, 2, scroll_height, SSD1306_WHITE);
    }
  }

  // Finish Up
  UI.drawButtons();
  UI.drawToast();
  UI.render();
}

void UIMenu::tick() {
  if (UI.toastRenderPending()) {
    if (!UI.hasToast()) {
      render();
    } else {
      UI.drawToast();
      UI.render();
    }
  }
}

void UIMenu::open(UIMenu *previous, bool save_history, void *arg) {
  this->current_arg = arg;
  log_i("Menu open with arg=%x", arg);
  initialize();

  if (on_open != nullptr) {
    on_open(this);
  }

  if (save_history) {
    prev = previous;
  }

  UI.clearButtons();
  UI.setButton(0, "BACK");
  UI.setButton(2, "ENTER");
  render();
}

void UIMenu::setTitle(const char *text) {
  strncpy(this->title, text, TITLE_SIZE);
}

UIMenu *UIMenu::close() {
  if (on_close != nullptr) {
    on_close(this);
  }
  clearItems();
  return prev;
}

void UIMenu::selectNext() {
  if (millis() - last_menu_change < MENU_SCROLL_DEBOUNCE) {
    return; // debounce
  } else {
    last_menu_change = millis();
  }

  if (current_item == nullptr || current_item->next == nullptr) {
    return;
  }

  current_item = current_item->next;
  render();
}

void UIMenu::selectPrev() {
  if (millis() - last_menu_change < MENU_SCROLL_DEBOUNCE) {
    return; // debounce
  } else {
    last_menu_change = millis();
  }

  if (current_item == nullptr || current_item->prev == nullptr) {
    return;
  }

  current_item = current_item->prev;
  render();
}

void UIMenu::handleClick() {
  if (current_item != nullptr) {
    if (current_item->cb != nullptr) {
      current_item->cb(this);
    } else if (current_item->pcb != nullptr) {
      current_item->pcb(this, current_item->arg);
    } else if (current_item->ipcb != nullptr) {
      current_item->ipcb(this, current_item->iarg);
    }
  }
}

void UIMenu::clearItems() {
  UIMenuItem *item = first_item;
  while (item != nullptr) {
    UIMenuItem *tmp = item;
    item = item->next;
    free(tmp);
  }
  first_item = nullptr;
  current_item = nullptr;
  last_item = nullptr;
}

UIMenuItem *UIMenu::getNthItem(int n) {
  UIMenuItem *p = first_item;
  while (--n > 0 && p != nullptr) {
    p = p->next;
  }
  return p;
}

void UIMenu::initialize(bool reinit) {
  int position = -1;

  if (reinit) {
    position = getCurrentPosition();
  }

  clearItems();
  initializer(this);

  if (!reinit) {
    current_item = first_item;
  } else {
    if (position > getItemCount()) {
      current_item = last_item;
    } else {
      current_item = getNthItem(position);
    }
  }
}

int UIMenu::getItemCount() {
  int menu_item_count = 0;
  UIMenuItem *item = first_item;
  while (item != nullptr) {
    menu_item_count++;
    item = item->next;
  }

  return menu_item_count;
}

int UIMenu::getCurrentPosition() {
  int menu_item_count = 0;
  int menu_item_position = 0;
  UIMenuItem *item = first_item;
  while (item != nullptr) {
    menu_item_count++;
    if (item == current_item) {
      menu_item_position = menu_item_count;
      break;
    }
    item = item->next;
  }

  return menu_item_position;
}

void UIMenu::onOpen(MenuCallback cb) {
  on_open = cb;
}

void UIMenu::onClose(MenuCallback cb) {
  on_close = cb;
}

void UIMenu::rerender() {
  initialize(true);
  render();
}