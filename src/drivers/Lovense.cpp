#include "drivers/Lovense.h"

using namespace BluetoothDriver;

static const char* TAG = "BluetoothDriver::Lovense";

bool Lovense::setSpeed(uint8_t speed) {
    // Lovense takes speeds 0..20, and dividing by 13 with the 255 check is good enough
    uint8_t cmdArg = speed == 255 ? 20 : speed / 13;

    if (this->sendf("Vibrate:%d;", cmdArg)) {
        // return this->waitForNotify();
    }

    return false;
}

bool Lovense::send(const char* cmd) {
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

bool Lovense::sendf(const char* fmt, ...) {
    char cmd[CMD_MAX_LEN + 1] = "";
    va_list args;
    va_start(args, fmt);
    vsniprintf(cmd, CMD_MAX_LEN, fmt, args);
    va_end(args);

    return send(cmd);
}

size_t Lovense::read(char* buf, size_t len, bool waitForNotify) {
    if (waitForNotify) {
        this->waitForNotify();
    }

    std::string val = this->rxChar->readValue();
    strncpy(buf, val.c_str(), len);
}

bool Lovense::waitForNotify() {
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

Device* Lovense::detect(NimBLEAdvertisedDevice* device, NimBLEClient* client, NimBLERemoteService* service) {
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
        Lovense* d = new Lovense(device->getName().c_str(), client, rxChar, txChar);
        return (BluetoothDriver::Device*) d;
    }
}