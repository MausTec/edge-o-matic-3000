#include "../../include/UIMenu.h"
#include "../../include/UserInterface.h"
#include "../../include/Page.h"

static void buildMenu(UIMenu *menu) {
  menu->addItem("Automatic Edging", &RunGraphPage);

  menu->addItem(&EdgingSettingsMenu);
  menu->addItem(&UISettingsMenu);
  menu->addItem(&NetworkMenu);
  menu->addItem(&GamesMenu);
}

UIMenu MainMenu("Main Menu", &buildMenu);
