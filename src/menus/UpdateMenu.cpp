#include "UIMenu.h"
#include "UpdateHelper.h"
#include "UserInterface.h"

#include <SD.h>

static void onCheckForUpdates(UIMenu *menu) {
  UI.toastNow("Checking...", 0, false);
  UpdateHelper::checkForUpdates();
  menu->initialize();

  if (UpdateHelper::pendingLocalUpdate || UpdateHelper::pendingWebUpdate) {
    UI.toastNow("Updates found!", 3000);
  } else {
    UI.toastNow("No Updates.", 3000);
  }

  menu->render();
}

static void onWebUpdate(UIMenu *menu) {
  UI.toastNow("Connecting...", 0, false);
  UpdateHelper::updateFromWeb();
}

static void onLocalUpdate(UIMenu *menu) {
  UI.toastNow("Preparing...", 0, false);
  UpdateHelper::updateFromFS(SD);
}

static void buildMenu(UIMenu *menu) {
  menu->addItem("Check for Updates...", &onCheckForUpdates);

  if (UpdateHelper::pendingWebUpdate) {
    menu->addItem("Update from Web", &onWebUpdate);
  }

  if (UpdateHelper::pendingLocalUpdate) {
    menu->addItem("Update from SD", &onLocalUpdate);
  }
}

UIMenu UpdateMenu("Update", &buildMenu);