#include "../../include/UIMenu.h"

UIMenu MainMenu("Main Menu", [](UIMenu *menu) {
  menu->addItem("Test Item", []() {
    Serial.println("Test Item Callback");
  });

  menu->addItem("Another Item", []() {
    Serial.println("Another Test Item Callback");
  });

  for (int i = 0; i < 4; i++) {
    menu->addItem("ITEM");
  }
});
