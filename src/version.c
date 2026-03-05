#include "version.h"
#include "esp_log.h"

static const char* TAG = "version";

const char* EOM_VERSION =
#ifdef PROJECT_VER
    PROJECT_VER
#else
    "0.0.0"
#endif
    ;

semver_t EOM_PARSED_VERSION = {};
bool EOM_VERSION_VALID = false;

void version_init(void) {
    semver_free(&EOM_PARSED_VERSION);
    EOM_VERSION_VALID = false;

    const char* ver_str = EOM_VERSION + (EOM_VERSION[0] == 'v' ? 1 : 0);
    if (semver_parse(ver_str, &EOM_PARSED_VERSION) == -1) {
        ESP_LOGW(TAG, "Could not parse EOM_VERSION as SemVer: %s", EOM_VERSION);
    } else {
        EOM_VERSION_VALID = true;
        ESP_LOGI(
            TAG,
            "Version: %d.%d.%d pre=%s build=%s",
            EOM_PARSED_VERSION.major,
            EOM_PARSED_VERSION.minor,
            EOM_PARSED_VERSION.patch,
            EOM_PARSED_VERSION.prerelease ? EOM_PARSED_VERSION.prerelease : "(none)",
            EOM_PARSED_VERSION.metadata ? EOM_PARSED_VERSION.metadata : "(none)"
        );
    }
}