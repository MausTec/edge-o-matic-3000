#include "system/event_manager.h"
#include <esp_log.h>
#include <string.h>

static const char* TAG = "system:event_manager";

const char* EVT_ALL = "";
EVENTS(EVENT_DEFINE);

static event_node_t* _handlers = NULL;

/**
 * @brief Returns the handlers linked list attached to a specific event tag, case sensitive!
 *
 * If that event tag is not already in the current list, it will be added by this call.
 *
 * @param event Event string
 * @return event_handler_node_t* Linked list of event handler nodes.
 */
static event_node_t* _get_handlers(const char* event) {
    event_node_t* ptr = _handlers;

    while (ptr != NULL) {
        if (!strcmp(ptr->event, event)) return ptr;
        if (ptr->next == NULL) break;
        ptr = ptr->next;
    }

    event_node_t* node = malloc(sizeof(event_node_t) + strlen(event) + 1);
    if (node == NULL) return NULL;

    node->handlers = NULL;
    node->next = NULL;
    strcpy(node->event, event);

    if (ptr == NULL)
        _handlers = node;
    else
        ptr->next = node;

    return node;
}

void event_manager_dispatch_synthetic(
    const char* match_event,
    const char* event,
    EVENT_HANDLER_ARG_TYPE event_arg_ptr,
    int event_arg_int
) {
    ESP_LOGD(
        TAG,
        "event_manager_dispatch(\"%s\", \"%s\", %p, %d);",
        match_event,
        event,
        event_arg_ptr,
        event_arg_int
    );

    event_node_t* event_node = _get_handlers(match_event);
    if (event_node == NULL) return;

    event_handler_node_t* ptr = event_node->handlers;

    while (ptr != NULL) {
        if (ptr->handler != NULL)
            ptr->handler(event, event_arg_ptr, event_arg_int, ptr->handler_arg);
        ptr = ptr->next;
    }

    // Also, dispatch for "All" handlers:
    if (match_event != EVT_ALL) {
        event_manager_dispatch_synthetic(EVT_ALL, event, event_arg_ptr, event_arg_int);
    }
}

void event_manager_dispatch(
    const char* event, EVENT_HANDLER_ARG_TYPE event_arg_ptr, int event_arg_int
) {
    event_manager_dispatch_synthetic(event, event, event_arg_ptr, event_arg_int);
}

event_handler_node_t* event_manager_register_handler(
    const char* event, event_handler_t handler, EVENT_HANDLER_ARG_TYPE handler_arg
) {
    event_node_t* event_node = _get_handlers(event);
    if (event_node == NULL) return NULL;

    event_handler_node_t* ptr = event_node->handlers;

    while (ptr && ptr->next != NULL)
        ptr = ptr->next;

    event_handler_node_t* node = malloc(sizeof(event_handler_node_t));
    if (node == NULL) return NULL;

    node->handler = handler;
    node->handler_arg = handler_arg;
    node->next = NULL;

    if (ptr == NULL)
        event_node->handlers = node;
    else
        ptr->next = node;

    return node;
}