#include "UIMenu.h"
#include "UserInterface.h"
#include "update_manager.h"

static um_update_status_t _status = UM_UPDATE_NOT_CHECKED;

static void onCheckForUpdates(UIMenu *menu) {
  UI.toastNow("Checking...", 0, false);
  _status = update_manager_check_for_updates();
  menu->initialize();

  if (_status == UM_UPDATE_AVAILABLE) {
    UI.toastNow("Updates found!", 3000);
  } else {
    UI.toastNow("No Updates.", 3000);
  }

  menu->render();
}

static void onWebUpdate(UIMenu *menu) {
  UI.toastNow("Connecting...", 0, false);
  esp_err_t err = update_manager_update_from_web();
  
  if (err != ESP_OK) {
    UI.toastNow("Update failed.");
  } else {
    UI.toastNow("Update Installed.");
  }
}

static void onLocalUpdate(UIMenu *menu) {
  UI.toastNow("Preparing...", 0, false);
}

static void buildMenu(UIMenu *menu) {
  menu->addItem("Check for Updates...", &onCheckForUpdates);

  if (_status == UM_UPDATE_AVAILABLE) {
    menu->addItem("Update from Web", &onWebUpdate);
  }

  if (_status == UM_UPDATE_LOCAL_FILE) {
    menu->addItem("Update from SD", &onLocalUpdate);
  }
}

UIMenu UpdateMenu("Update", &buildMenu);