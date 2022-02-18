#ifndef __SDHelper_h
#define __SDHelper_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdio.h>
#include "cJSON.h"
#include "esp_vfs.h"
#include "console.h"

/**
 *  @deprecated Use PATH_MAX
 */
#define SD_MAX_DIR_LENGTH PATH_MAX

/**
 * Fills *buf with an absolute path to *path, prefixing the mount point of the SD card as defined
 * by the hardware driver.
 * 
 * @returns Populated size of the resulting buffer. If len == 0 or *buf is NULL, the size will be
 *          calculated but no assignments made.
 */
size_t SDHelper_getAbsolutePath(char *buf, size_t len, const char *path);

/**
 * 
 */
size_t SDHelper_getRelativePath(char *path, size_t len, const char *argv, console_t *console);

/**
 * Joins a path suffix *path to the path in *buf, up to len characters.
 * 
 * @returns Populated size of the resulting buffer. If len == 0 or *buf is NULL, the size will be
 *          calculated but no assignments made.
 */
size_t SDHelper_join(char *buf, size_t len, const char *path);


void SDHelper_printDirectory(DIR *dir, int numTabs, char *out, size_t len);
void SDHelper_printDirectoryJson(DIR *dir, cJSON *files);
void SDHelper_logDirectory(const char *path);
void SDHelper_logFile(const char *path);
void SDHelper_printFile(FILE *file, char *out, size_t len);

#ifdef __cplusplus
}
#endif

#endif
