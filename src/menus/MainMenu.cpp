#include "UIMenu.h"
#include "UserInterface.h"
#include <string>

static void buildMenu(UIMenu *menu) {
  // menu->addItem("Automatic Edging", &RunGraphPage);

  menu->addItem(&EdgingSettingsMenu);
  menu->addItem(&EdgingOrgasmSettingsMenu);
  menu->addItem(&UISettingsMenu);
  menu->addItem(&ConfigMenu);
  menu->addItem(&NetworkMenu);
  menu->addItem(&GamesMenu);
  menu->addItem(&UpdateMenu);

  menu->addItem("System Info", [](UIMenu*) {
    // UI.toastNow(std::string("S/N: ") + Hardware::getDeviceSerial() + "\n" + "Version: " VERSION);
  });
}

UIMenu MainMenu("Main Menu", &buildMenu);
