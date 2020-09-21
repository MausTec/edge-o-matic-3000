#include "../include/UIMenu.h"
#include "../include/UserInterface.h"

UIMenu::UIMenu(char *t, void(*fn)(UIMenu *)) {
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

    int menu_item_count = 0;
    int current_item_position = 0;
    item = first_item;
    while (item != nullptr) {
      menu_item_count++;
      if (item == current_item) {
        current_item_position = menu_item_count;
      }
      item = item->next;
    }

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
  UI.render();
}

void UIMenu::open(UIMenu *previous, bool save_history) {
  if (save_history)
    prev = previous;
  current_item = first_item;
  UI.clearButtons();
  UI.setButton(0, "BACK");
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