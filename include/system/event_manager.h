#ifndef __system__event_manager_h
#define __system__event_manager_h

#ifdef __cplusplus
extern "C" {
#endif

#include "system/events.h"

#define EVENT_DECL(evt) const char* evt
#define EVENT_DEFINE(evt) const char* evt = #evt

#define EVENT_HANDLER_ARG_TYPE void*

EVENTS(EVENT_DECL);

typedef void (*event_handler_t)(void);

/**
 * @brief Registers an event handler.
 *
 * Event handlers may only be registered once per event. This function will filter through the
 * existing event handlers, and if such an event/handler combination is found, will return an error.
 *
 * @param event Event string
 * @param handler Handler function pointer
 * @param handler_arg Argument to pass along to the handler_arg param in Handler function
 * invocations
 */
void event_manager_register_handler(
    const char* event, event_handler_t handler, EVENT_HANDLER_ARG_TYPE handler_arg
);

/**
 * @brief Unregisters a handler from a specific event.
 *
 * @param event Event string
 * @param handler Handler function pointer
 */
void event_manager_unregister_handler(const char* event, event_handler_t handler);

/**
 * @brief Unregisters a handler from all events.
 *
 * @param handler Handler function pointer
 */
void event_manager_unregister_handler_all(event_handler_t handler);

/**
 * @brief Dispatches an event to all handlers registered to it.
 *
 * @param event Event string
 * @param event_arg_ptr Event arg, pointer
 * @param event_arg_int Event arg, integer
 */
void event_manager_dispatch(
    const char* event, EVENT_HANDLER_ARG_TYPE event_arg_ptr, int event_arg_int
);

#ifdef __cplusplus
}
#endif

#endif
