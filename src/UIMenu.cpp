#include "../include/UIMenu.h"

UIMenu::UIMenu(char *t, void(*fn)(UIMenu*)) {
  title = t;
  Serial.println("Initialized menu: " + String(t));
  fn(this);
}

void UIMenu::addItem(char *text, MenuCallback cb) {
  Serial.println("Adding item: " + String(text));
}

void UIMenu::render() {
  Serial.println("Rendering a wonderful menu!!");
}

void UIMenu::open(UIMenu *previous) {
  Serial.println("Opening menu: " + String(title));
  prev = previous;
}

UIMenu *UIMenu::close() {
  Serial.println("Closing menu: " + String(title));
  return prev;
}