#include "util/i18n.h"
#include "esp_log.h"

static const char* TAG = "i18n";

const char* _(const char* str) {
    ESP_LOGI(TAG, "Lookup \"%s\" (addr 0x%p): \"%s\"", str, (void*)str, "-");
    return str;
}