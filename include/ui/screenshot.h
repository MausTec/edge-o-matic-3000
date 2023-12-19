#ifndef __ui__screenshot_h
#define __ui__screenshot_h

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Save a screenshot to a file as an XBMP file.
 *
 * This is not thread safe, but the implementation might end up with a mutex in it to be sure. You
 * can blame u8g2's lack of a user pointer in the screenshot write function call. Consider this...
 * Schrödinger's Thread Safety. It is and is not until it crashes and the core dump can be observed.
 *
 * @param filename Path to the resulting file, absolute, plz. This can be NULL, btw, in which case a
 * filename will be generated.
 * @return size_t Size of the data written to the file. If this is 0 you may have an issue.
 */
size_t ui_screenshot_save(const char* filename);

#ifdef __cplusplus
}
#endif

#endif
