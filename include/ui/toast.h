#ifndef __ui__toast_h
#define __ui__toast_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define TOAST_MAX 256

/**
 * @brief Adds a toast to the toasty queue.
 * 
 * @param fmt 
 * @param ... 
 */
void ui_toast(const char *fmt, ...);

/**
 * @brief Adds a toast to the toasty queue which cannot be dismissed.
 * 
 * @param fmt 
 * @param ... 
 */
void ui_toast_blocking(const char *fmt, ...);

/**
 * @brief Clears any active toasts.
 */
void ui_toast_clear(void);

/**
 * @brief Renders the current toast, if one should be shown.
 */
void ui_toast_render(void);

/**
 * @brief Checks if a toast is currently active and rendering.
 * 
 * @return 1 Toast present
 * @return 0 Toast not present obviously why do we document booleans
 */
int ui_toast_is_active(void);

/**
 * @brief Checks if a toast can be dismissed by the user.
 * 
 * @return 1 Button press can dismiss a toast.
 * @return 0 Toast can only be dismissed by code.
 */
int ui_toast_is_dismissable(void);

/**
 * @brief Returns the current toast string, if one is set.
 * 
 * @return const char* Will be an empty string if no toast is set.
 */
const char* ui_toast_get_str(void);

#ifdef __cplusplus
}
#endif

#endif
