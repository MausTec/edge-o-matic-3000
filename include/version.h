#ifndef __edge_o_matic__version_h
#define __edge_o_matic__version_h

#ifdef __cplusplus
extern "C" {
#endif

#include "semver.h"
#include <stdbool.h>

/**
 * Raw version string as compiled in. May include SemVer build metadata
 * (e.g. "2.0.0-rc.1+abc12345.feature.foo"). Use EOM_PARSED_VERSION for
 * structured access after calling version_init().
 */
extern const char* EOM_VERSION;

/**
 * Parsed semver_t populated by version_init(). Valid only when
 * EOM_VERSION_VALID is true.
 */
extern semver_t EOM_PARSED_VERSION;

/**
 * True if EOM_VERSION was successfully parsed as a SemVer string.
 * If False, assume the current version is an uncomparable internal build and allow updates to
 * proceed.
 */
extern bool EOM_VERSION_VALID;

/**
 * Parse EOM_VERSION into EOM_PARSED_VERSION. Call once early in app_main.
 */
void version_init(void);

#ifdef __cplusplus
}
#endif

#endif
