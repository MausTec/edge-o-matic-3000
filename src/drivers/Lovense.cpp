#include "drivers/Lovense.h"

using namespace BluetoothDriver;

static const char *TAG = "BluetoothDriver::Lovense";

void Lovense::setSpeed(uint8_t speed) {
    char cmd[20] = "";

    // Lovense takes speeds 0..20, and dividing by 13 with the 255 check is good enough
    uint8_t cmdArg = speed == 255 ? 20 : speed / 13;
    snprintf(cmd, 20, "Vibrate:%d;", cmdArg);

    if (!this->txChar->writeValue(cmd)) {
        ESP_LOGE(TAG, "Write failed, disconnect.");
        this->disconnect();
        return;
    }

    ESP_LOGE(TAG, "Send command: \"%s\"\n", cmd);
}