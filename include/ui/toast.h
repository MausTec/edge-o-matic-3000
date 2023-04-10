#ifndef __ui__toast_h
#define __ui__toast_h

#ifdef __cplusplus
extern "C" {
#endif

#include "u8g2.h"
#include <stdint.h>

#define TOAST_MAX 256

/**
 * @brief Adds a toast to the toasty queue.
 *
 * This will format and copy the string, so you can free up your buffer when you're done.
 *
 * @param fmt
 * @param ...
 */
void ui_toast(const char* fmt, ...);

/**
 * @brief Toast option for longer text fields. Your buffer, your problem.
 *
 * This will automatically word wrap as it renders, and allow the user to scroll the toast message.
 * This doesn't copy anything to an internal buffer, so you are responsible for the lifecycle of the
 * buffer passed.
 *
 * @param msg
 */
void ui_toast_multiline(const char* msg);

/**
 * @brief Appends text to the current toast.
 *
 * This will format and copy the string, so you can free up your buffer when you're done.
 *
 * @param fmt
 * @param ...
 */
void ui_toast_append(const char* fmt, ...);

/**
 * @brief This sets the progress of the current toast, or an indeterminate bar.
 *
 * If the current toast does not have a progress bar attached, this will append one.
 *
 * @param progress The current progress value
 * @param max The maximum target value, or 0 for indeterminate progress
 */
void ui_toast_set_progress(int progress, int max);

/**
 * @brief Adds a toast to the toasty queue which cannot be dismissed. Renders immediately.
 *
 * This will format and copy the string, so you can free up your buffer when you're done.
 * This forces a rerender in the current task to display the toast immediately.
 *
 * @param fmt
 * @param ...
 */
void ui_toast_blocking(const char* fmt, ...);

/**
 * @brief Assign a callback for to call when closing this toast.
 *
 * @param cb Callback.
 * @param arg Argument to give that callback aren't I so generous?
 */
void ui_toast_on_close(void (*cb)(void*), void* arg);

/**
 * @brief Clears any active toasts.
 */
void ui_toast_clear(void);

/**
 * @brief Call before clearing the toast. This will run the on_close arg.
 *
 */
void ui_toast_handle_close(void);

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
 * @brief Draws the toast frame and shadow, useful for creating other dialog boxes, like inputs.
 *
 * @param d
 * @param margin
 * @param start_x
 * @param start_y
 */
void ui_toast_draw_frame(
    u8g2_t* d, uint8_t margin, uint8_t start_x, uint8_t start_y, uint8_t height
);

/**
 * @brief Returns the current toast string, if one is set.
 *
 * This will return either the internal buffer for a normal toast, or the multiline buffer for
 * a multiline toast.
 *
 * @return const char* Will be an empty string if no toast is set.
 */
const char* ui_toast_get_str(void);

#ifdef __cplusplus
}
#endif

#endif
