#include "../../include/UIMenu.h"
#include "../../include/UpdateHelper.h"
#include "../../include/UserInterface.h"

#include <SD.h>

static void onCheckForUpdates(UIMenu *menu) {
  UI.toastNow("Checking...", 0, false);
  UpdateHelper::checkForUpdates();
  menu->initialize();

  if (UpdateHelper::pendingLocalUpdate || UpdateHelper::pendingWebUpdate) {
    UI.toastNow("Updates found!");
  } else {
    UI.toastNow("No Updates.");
  }
}

static void onWebUpdate(UIMenu *menu) {
  UI.toastNow("Not Implemented.");
}

static void onLocalUpdate(UIMenu *menu) {
  UI.toastNow("Updating...", 0, false);
  UpdateHelper::updateFromFS(SD);
}

static void buildMenu(UIMenu *menu) {
  menu->addItem("Check for Updates...", &onCheckForUpdates);

  if (UpdateHelper::pendingWebUpdate) {
    menu->addItem("Update from Web");
  }

  if (UpdateHelper::pendingLocalUpdate) {
    menu->addItem("Update from SD");
  }
}

UIMenu UpdateMenu("Update", &buildMenu);