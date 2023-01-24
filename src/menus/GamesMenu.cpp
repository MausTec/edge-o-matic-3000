#include "UIMenu.h"
#include "Page.h"
#include "application.h"


#include "esp_log.h"
static const char *TAG = "GamesMenu";

UIMenu GamesMenu("Apps", [](UIMenu *menu) {
  menu->enableAutoCleanup(AUTOCLEAN_FREE);

  menu->addItem("Snake", [](UIMenu*) {
      
  });

  DIR* d;
  struct dirent* dir;
  const char *path = eom_hal_get_sd_mount_point();
  d = opendir(path);

  if (d) {
      while ((dir = readdir(d)) != NULL) {
          if (dir->d_type == DT_REG && !strcmp(dir->d_name + strlen(dir->d_name) - 4, ".zip")) {
            char *file = (char*) malloc(strlen(dir->d_name) + strlen(path) + 2);
            if (file == NULL) break;
            sprintf(file, "%s/%s", path, dir->d_name);

            menu->addItem(dir->d_name, [](UIMenu*, void* name) {
              UI.toastNow("Loading...", 0, false);
              application_t *app;
              app_err_t err = application_load((const char*) name, &app);
              UI.toastNow("Loaded.");
              if (err == APP_OK) application_start(app);
              else {
                UI.toastNow("App Load Error", 0);
                ESP_LOGW(TAG, "App Err: %d", err);
              }
            }, file);
          }
      }

      closedir(d);
  }
});