#include "UIMenu.h"
#include "Page.h"

UIMenu GamesMenu("Apps", [](UIMenu *menu) {
  menu->addItem("Snake", [](UIMenu*) {
    Page::Go(&SnakePage);
  });
});