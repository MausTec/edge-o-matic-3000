#include "../../include/UIMenu.h"
#include "../../include/UserInterface.h"

UIMenu WiFiMenu("WiFi Settings", [](UIMenu *menu) {
  menu->addItem("Enable WiFi", []() {
    UI.toast("Not Implemented", 3000);
  });

  menu->addItem("Connection Status", []() {
    UI.toast("Status!");
  });
});

UIMenu MainMenu("Main Menu", [](UIMenu *menu) {
  menu->addItem("WiFi Settings", []() {
    UI.openMenu(&WiFiMenu);
  });
});
