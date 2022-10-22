#include "drivers/Nobra.h"
#include <cmath>

using namespace BluetoothDriver;

static const char* TAG = "BluetoothDriver::Nobra";

char Nobra::getSpeed(uint8_t speed) {
    const uint8_t nobraSpeedResolution = 16;
    float proportion = speed / 255;
    uint8_t nobraSpeed = round(nobraSpeedResolution * proportion);

    if (nobraSpeed == 0) {
        return 'p';
    }

    char output = 'a' - 1;
    output += nobraSpeed;
    return output;
}

bool Nobra::setSpeed(uint8_t speed) {
    char speedCommand = getSpeed(speed);
    if (this->sendf("%X", speedCommand)) {
        return this->waitForNotify();
    }

    return false;
}

bool Nobra::send(const char* cmd) {
    if (!this->isConnected()) {
        ESP_LOGE(TAG, "Client was disconnected.");
        return false;
    }

    std::string cmdStr(cmd);
    if (!this->txChar->writeValue(cmdStr)) {
        ESP_LOGE(TAG, "Write failed, disconnect.");
        this->disconnect();
        return false;
    }

    ESP_LOGD(TAG, "- LVS txChar = %s", this->txChar->getUUID().toString().c_str());
    ESP_LOGI(TAG, "Send command: \"%s\"", cmd);
    return true;
}

bool Nobra::sendf(const char* fmt, ...) {
    char cmd[CMD_MAX_LEN + 1] = "";
    va_list args;
    va_start(args, fmt);
    vsniprintf(cmd, CMD_MAX_LEN, fmt, args);
    va_end(args);

    return send(cmd);
}

size_t Nobra::read(char* buf, size_t len, bool waitForNotify) {
    if (waitForNotify) {
        this->waitForNotify();
    }

    std::string val = this->rxChar->readValue();
    strncpy(buf, val.c_str(), len);
}

bool Nobra::waitForNotify() {
    unsigned long start_ms = esp_timer_get_time() / 1000UL;
    unsigned long curr_ms = start_ms;

    do {
        curr_ms = esp_timer_get_time() / 1000UL;
        vTaskDelay(10 / portTICK_RATE_MS);
    } while (curr_ms - start_ms < WAIT_FOR_NOTIFY_TIMEOUT_MS && !notifyPending);

    bool result = notifyPending;
    notifyPending = false;
    return notifyPending;
}

Device* Nobra::detect(NimBLEAdvertisedDevice* device, NimBLEClient* client, NimBLERemoteService* service) {
    std::vector<NimBLERemoteCharacteristic*>* chars = service->getCharacteristics(true);

    NimBLERemoteCharacteristic* rxChar = nullptr;
    NimBLERemoteCharacteristic* txChar = nullptr;

    for (NimBLERemoteCharacteristic* c : *chars) {
        if (c->canNotify()) {
            rxChar = c;
        }

        if (c->canWrite()) {
            txChar = c;
        }
    }

    if (txChar != nullptr && rxChar != nullptr) {
        // found two chars, tx & rx, and we can write.
        ESP_LOGE(TAG, "- LVS rxChar = %s", rxChar->getUUID().toString().c_str());
        ESP_LOGE(TAG, "- LVS txChar = %s", txChar->getUUID().toString().c_str());
        Nobra* d = new Nobra(device->getName().c_str(), client, rxChar, txChar);
        return (BluetoothDriver::Device*) d;
    }
}