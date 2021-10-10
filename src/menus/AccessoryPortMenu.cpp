#include "UIMenu.h"
#include "UserInterface.h"
#include "Hardware.h"

static void buildMenu(UIMenu *menu) {
  menu->addItem("Disable", [](UIMenu*) { Hardware::disableExternalBus(); });
  menu->addItem("Enable", [](UIMenu*) { Hardware::enableExternalBus(); });

#ifdef I2C_SLAVE_ADDR
  menu->addItem("Slave", [](UIMenu*) {
    Hardware::joinI2c(I2C_SLAVE_ADDR);
    Hardware::enableExternalBus();
  });
#endif
}

UIMenu AccessoryPortMenu("Accessory Port", &buildMenu);