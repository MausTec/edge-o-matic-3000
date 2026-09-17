#include "ui/control_lock.h"
#include "eom-hal.h"
#include "esp_log.h"
#include "polyfill.h"
#include <string.h>

static const char* TAG = "ui:control_lock";

// Hold BACK+OK together for this long to force-unlock, regardless of what
// engaged the lock or why.
#define ESCAPE_COMBO (EOM_HAL_BUTTON_BACK | EOM_HAL_BUTTON_OK)
#define ESCAPE_HOLD_MS 5000UL
#define REASON_MAX 64

static struct {
    bool locked;
    char reason[REASON_MAX];
    unsigned long combo_start_ms;
} _lock;

void control_lock_init(void) {
    _lock.locked = false;
    _lock.reason[0] = '\0';
    _lock.combo_start_ms = 0;
}

void control_lock_engage(const char* reason) {
    _lock.locked = true;

    if (reason) {
        strncpy(_lock.reason, reason, REASON_MAX - 1);
        _lock.reason[REASON_MAX - 1] = '\0';
    } else {
        _lock.reason[0] = '\0';
    }

    ESP_LOGI(TAG, "Controls locked: %s", _lock.reason);
}

void control_lock_release(void) {
    if (!_lock.locked) return;

    _lock.locked = false;
    _lock.reason[0] = '\0';
    ESP_LOGI(TAG, "Controls unlocked");
}

bool control_lock_is_locked(void) {
    return _lock.locked;
}

const char* control_lock_get_reason(void) {
    return _lock.reason;
}

void control_lock_tick(void) {
    uint8_t state = eom_hal_get_button_state();

    if ((state & ESCAPE_COMBO) != ESCAPE_COMBO) {
        _lock.combo_start_ms = 0;
        return;
    }

    if (_lock.combo_start_ms == 0) {
        _lock.combo_start_ms = millis();
        return;
    }

    if (_lock.locked && millis() - _lock.combo_start_ms >= ESCAPE_HOLD_MS) {
        ESP_LOGW(TAG, "Escape-hatch combo held: force unlocking");
        control_lock_release();
    }
}
