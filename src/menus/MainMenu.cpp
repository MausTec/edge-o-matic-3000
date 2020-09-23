#include "../../include/UIMenu.h"
#include "../../include/UserInterface.h"
#include "../../include/Page.h"

void onAutoClick(UIMenu*) {
  Page::Go(&RunGraphPage);
}

void onNetworkClick(UIMenu*) {
  UI.openMenu(&NetworkMenu);
}

void onGamesClick(UIMenu*) {
  UI.openMenu(&GamesMenu);
}

void buildMenu(UIMenu *menu) {
  menu->addItem("Automatic Edging", onAutoClick);

  menu->addItem("Edging Settings");
  menu->addItem("UI Settings");

  menu->addItem("Network Settings", onNetworkClick);
  menu->addItem("Games", onGamesClick);
}

UIMenu MainMenu("Main Menu", &buildMenu);
