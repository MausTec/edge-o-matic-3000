#include "system/event_manager.h"
#include <esp_log.h>

static const char* TAG = "system:event_manager";

EVENTS(EVENT_DEFINE);

void event_manager_dispatch(
    const char* event, EVENT_HANDLER_ARG_TYPE event_arg_ptr, int event_arg_int
) {
    ESP_LOGI(TAG, "event_manager_dispatch(\"%s\", %p, %d);", event, event_arg_ptr, event_arg_int);
}