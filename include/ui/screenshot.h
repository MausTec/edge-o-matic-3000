#ifndef __ui__screenshot_h
#define __ui__screenshot_h

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Save a screenshot to a file as an BMP file.
 *
 * @param filename Path to the resulting file, absolute, plz. This can be NULL, btw, in which case a
 * filename will be generated.
 * @return size_t Size of the data written to the file. If this is 0 you may have an issue.
 */
size_t ui_screenshot_save(const char* filename);

/**
 * @brief Save a screenshot to a temporary file and write out the generated filename as above.
 *
 * @param filename Destination pointer to filename buffer
 * @param len maximum write size of the buffer
 * @return size_t Nothing right now.
 */
size_t ui_screenshot_save_p(char* filename, size_t len);

#ifdef __cplusplus
}
#endif

#endif
