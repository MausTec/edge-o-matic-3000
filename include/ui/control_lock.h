#ifndef __ui__control_lock_h
#define __ui__control_lock_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief General-purpose input lock: while engaged, all button/encoder input
 * is suppressed at the UI dispatch layer (see ui.c's handle_button/handle_encoder).
 */

void control_lock_init(void);
void control_lock_engage(const char* reason);
void control_lock_release(void);
bool control_lock_is_locked(void);
const char* control_lock_get_reason(void);

/**
 * @brief Poll for the BACK+OK escape-hatch hold combo.
 *
 * Must be called unconditionally on every UI tick (locked or not) so the
 * escape hatch itself can never be locked out.
 */
void control_lock_tick(void);

#ifdef __cplusplus
}
#endif

#endif
