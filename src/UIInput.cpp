#include "../include/UIInput.h"
#include "../include/UserInterface.h"

void UIInput::render() {
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
  UI.setButton(2, "SAVE", [&]() { this->handleClick(); });

  // More
  UI.display->setTextSize(2);
  int digits = floor(log10(current_value)) + 1;
  UI.display->setCursor((SCREEN_WIDTH / 2) - (max(digits, 1) * 6), 14);
  UI.display->print(current_value);
  UI.display->setTextSize(1);

  UI.drawBar(32, '\0', current_value, max_value);

  UI.drawButtons();
  UI.drawToast();
  UI.render();
}

void UIInput::setMax(int n) {
  max_value = n;
}
void UIInput::setStep(int n) {
  increment = n;
}
void UIInput::setValue(int n) {
  current_value = n;
  default_value = n;
}
void UIInput::onChange(InputCallback fn) {
  on_change = fn;
}
void UIInput::onConfirm(InputCallback fn) {
  on_confirm = fn;
}

int UIInput::getItemCount() {
  return max_value;
}
int UIInput::getCurrentPosition() {
  return current_value;
}
void UIInput::selectNext() {
  set_current(current_value + increment);
  if (on_change != nullptr) {
    on_change(current_value);
  }
  render();
}
void UIInput::selectPrev() {
  set_current(current_value - increment);
  if (on_change != nullptr) {
    on_change(current_value);
  }
  render();
}
void UIInput::handleClick() {
  default_value = current_value;
  if (on_confirm != nullptr) {
    on_confirm(current_value);
  }
  UI.closeMenu();
}
UIMenu *UIInput::close() {
  if (on_change != nullptr) {
    on_change(default_value);
  }
  return UIMenu::close();
}

void UIInput::set_current(int v) {
  current_value = min(max_value, max(min_value, v));
}