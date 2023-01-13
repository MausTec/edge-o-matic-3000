#ifndef __update_manager_h
#define __update_manager_h

#ifdef __cplusplus
extern "C" {
#endif

#include "semver.h"
#include "esp_err.h"

#define REMOTE_UPDATE_URL "http://us-central1-maustec-io.cloudfunctions.net/gh-release-embedded-bridge"
#define UPDATE_FILENAME "/update.bin"

enum um_update_status {
    UM_UPDATE_NOT_CHECKED,
    UM_UPDATE_IS_CURRENT,
    UM_UPDATE_AVAILABLE,
    UM_UPDATE_LOCAL_FILE,
};

typedef enum um_update_status um_update_status_t;

esp_err_t update_manager_update_from_web(void);
esp_err_t update_manager_check_latest_version(semver_t *version);
um_update_status_t update_manager_check_for_updates(void);
um_update_status_t update_manager_get_status(void);

#ifdef __cplusplus
}
#endif

#endif