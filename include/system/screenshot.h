#ifndef __screenshot_h
#define __screenshot_h

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool screenshot_save_to_file(char* outpath, size_t len);

#ifdef __cplusplus
}
#endif

#endif
