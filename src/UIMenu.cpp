#include "../include/UIMenu.h"

UIMenu::UIMenu(char *t) {
  title = t;
}

void UIMenu::addItem(char *text, MenuCallback cb) {
  Serial.println("Adding item: " + String(text));
}

void UIMenu::render() {
  Serial.println("Rendering a wonderful menu!!");
}