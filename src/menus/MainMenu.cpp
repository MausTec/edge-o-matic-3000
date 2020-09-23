#include "../../include/UIMenu.h"
#include "../../include/UserInterface.h"
#include "../../include/Page.h"

void buildMenu(UIMenu *menu) {
  menu->addItem("Automatic Edging", &RunGraphPage);

  menu->addItem("Edging Settings");
  menu->addItem("UI Settings");

  menu->addItem(&NetworkMenu);
  menu->addItem(&GamesMenu);
}

UIMenu MainMenu("Main Menu", &buildMenu);
