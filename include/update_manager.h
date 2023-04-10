#ifndef __update_manager_h
#define __update_manager_h

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "semver.h"

#define REMOTE_UPDATE_URL                                                                          \
    "http://us-central1-maustec-io.cloudfunctions.net/gh-release-embedded-bridge"
#define UPDATE_FILENAME "update.bin"

enum um_update_status {
    UM_UPDATE_NOT_CHECKED,
    UM_UPDATE_AVAILABLE,
    UM_UPDATE_NOT_AVAILABLE,
    UM_UPDATE_ERROR = 100,
};

typedef enum um_update_status um_update_status_t;

typedef struct um_update_info {
    char* version;
    char* date;
    char* time;
    char* path;
} um_update_info_t;

typedef void um_update_callback_t(const um_update_info_t* info, void* arg);

typedef enum um_progress_event {
    UM_PROG_PREFLIGHT_DATA,
    UM_PROG_PREFLIGHT_DONE,
    UM_PROG_PREFLIGHT_ERROR,
    UM_PROG_PAYLOAD_DATA,
    UM_PROG_PAYLOAD_DONE,
} um_progress_event_t;

typedef void um_progress_callback_t(um_progress_event_t evt, int current, int total, void* arg);

/**
 * @brief Install the latest version from online update service.
 *
 * @return esp_err_t
 */
esp_err_t update_manager_update_from_web(um_progress_callback_t* progress_cb, void* cb_arg);

/**
 * @brief Install the update.bin file off local storage.
 *
 * @return esp_err_t
 */
esp_err_t update_manager_update_from_storage(
    const char* path, um_progress_callback_t* progress_cb, void* cb_arg
);

/**
 * @brief Don't remember what this does tbh.
 *
 * @param version
 * @return esp_err_t
 */
esp_err_t update_manager_check_latest_version(semver_t* version);

/**
 * @brief Checks both online and local storage for updates.
 *
 * @return um_update_status_t
 */
um_update_status_t update_manager_check_for_updates(void);

/**
 * @brief Checks online for any applicable updates.
 *
 * @return um_update_status_t
 */
um_update_status_t update_manager_check_online_updates(void);

/**
 * @brief Checks available versions online and calls on_result with each one.
 *
 * @param on_result
 */
void update_manager_list_online_updates(um_update_callback_t* on_result);

/**
 * @brief Checks available versions on SD card and calls on_result with each one.
 *
 * @param on_result
 */
um_update_status_t update_manager_list_local_updates(um_update_callback_t* on_result, void* cb_arg);

/**
 * @brief Checks local storage for any applicable updates.
 *
 * @return um_update_status_t
 */
um_update_status_t update_manager_check_local_updates(void);

/**
 * @brief Returns the last update check result, used for drawing the update icon.
 *
 * @return um_update_status_t
 */
um_update_status_t update_manager_get_status(void);

#ifdef __cplusplus
}
#endif

#endif