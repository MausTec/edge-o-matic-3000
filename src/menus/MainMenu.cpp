#include "../../include/UIMenu.h"

UIMenu MainMenu("Main Menu", [](UIMenu *menu) {
  Serial.println("Initialized a menu?");
  menu->addItem("Test Item");
});
