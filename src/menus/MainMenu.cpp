#include "UIMenu.h"
#include "UserInterface.h"
#include "Page.h"
#include "UpdateHelper.h"

static void buildMenu(UIMenu *menu) {
  menu->addItem("Automatic Edging", &RunGraphPage);

  menu->addItem(&EdgingSettingsMenu);
  menu->addItem(&UISettingsMenu);
  menu->addItem(&NetworkMenu);
  menu->addItem(&GamesMenu);
  menu->addItem(&UpdateMenu);

  menu->addItem("System Info", [](UIMenu*) {
    UI.toastNow(String("S/N: ") + Hardware::getDeviceSerial() + "\n" + "Version: " VERSION);
  });
}

UIMenu MainMenu("Main Menu", &buildMenu);
