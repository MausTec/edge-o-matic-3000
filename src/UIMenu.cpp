#include "../include/UIMenu.h"
#include "../include/UserInterface.h"

UIMenu::UIMenu(char *t, void(*fn)(UIMenu*)) {
  title = t;
  fn(this);
}

void UIMenu::addItem(char *text, MenuCallback cb) {
  UIMenuItem *item = new UIMenuItem();
  item->text = text;
  item->next = nullptr;
  item->cb = cb;
  item->prev = last_item;

  if (last_item != nullptr) {
    last_item->next = item;
  }

  last_item = item;

  if (first_item == nullptr) {
    first_item = item;
  }
}

void UIMenu::render() {
  UI.clear(false);
  UI.drawStatus(title);
  UI.drawIcons();
  UI.display->drawLine(0, 9, SCREEN_WIDTH, 9, SSD1306_WHITE);

  // Menu items here
  UIMenuItem *item = current_item;
  for (int i = 0; i < 2; i++) {
    if (item == nullptr || item->prev == nullptr)
      break;
    item = item->prev;
  }

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
  }

  // Finish Up
  UI.drawButtons();
  UI.render();
}

void UIMenu::open(UIMenu *previous) {
  prev = previous;
  current_item = first_item;
  render();
}

UIMenu *UIMenu::close() {
  Serial.println("Closing menu: " + String(title));
  return prev;
}

void UIMenu::selectNext() {
  if (current_item == nullptr || current_item->next == nullptr) {
    current_item = first_item;
  } else {
    current_item = current_item->next;
  }
  render();
  // This is a debounce.
  delay(100);
}

void UIMenu::selectPrev() {
  if (current_item == nullptr || current_item->prev == nullptr) {
    current_item = last_item;
  } else {
    current_item = current_item->prev;
  }
  render();
  // This is a debounce.
  delay(100);
}

void UIMenu::handleClick() {
  if (current_item != nullptr && current_item->cb != nullptr) {
    current_item->cb();
  }
}