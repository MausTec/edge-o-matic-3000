#include "../../include/UIMenu.h"
#include "../../include/UserInterface.h"
#include "../../include/Hardware.h"

static void buildMenu(UIMenu *menu) {
  menu->addItem("Disable", [](UIMenu*) { Hardware::disableExternalBus(); });
  menu->addItem("Enable", [](UIMenu*) { Hardware::enableExternalBus(); });

  menu->addItem("Slave", [](UIMenu*) {
    Hardware::joinI2c(I2C_SLAVE_ADDR);
    Hardware::enableExternalBus();
  });
}

UIMenu AccessoryPortMenu("Accessory Port", &buildMenu);