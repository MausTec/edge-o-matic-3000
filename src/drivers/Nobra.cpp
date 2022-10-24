#include "drivers/Nobra.h"
#include <cmath>

using namespace BluetoothDriver;

static const char* TAG = "BluetoothDriver::Nobra";

char Nobra::mapSpeed(uint8_t speed) {
    const uint8_t nobraSpeedResolution = 15;
    float proportion = float(speed) / 255.0;
    uint8_t nobraSpeed = round(nobraSpeedResolution * proportion);
    if (nobraSpeed == 0) {
        return 'p';
    }
    char output = 'a' - 1 + nobraSpeed;
    return output;
}

bool Nobra::setSpeed(uint8_t speed) {
    char speedCommand = mapSpeed(speed);

    bool isShiftingGears = lastSentSpeed != speedCommand;
    if (!isShiftingGears) {
        return false;
    }

    return this->sendf("%c", speedCommand);
}

bool Nobra::send(const char* cmd) {
    if (!this->isConnected()) {
        ESP_LOGE(TAG, "Client was disconnected.");
        return false;
    }

    std::string cmdStr(cmd);
    if (!this->remote->writeValue(cmdStr, false)) {
        ESP_LOGE(TAG, "Write failed, disconnect.");
        this->disconnect();
        return false;
    }

    ESP_LOGD(TAG, "- remote = %s", this->remote->getUUID().toString().c_str());
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

Device* Nobra::detect(NimBLEAdvertisedDevice* device, NimBLEClient* client, NimBLERemoteService* service) {
    std::vector<NimBLERemoteCharacteristic*> *chars = service->getCharacteristics(true);
    NimBLERemoteCharacteristic *writeChar = nullptr;

    for (NimBLERemoteCharacteristic* c : *chars) {
        if (c->canWriteNoResponse()) {
            writeChar = c;
        }
    }
    const char *deviceName = device->getName().c_str();
    bool writeCharacteristicFound = writeChar != nullptr;
    bool looksLikeNobra = std::string(deviceName).find("NobraControl") != std::string::npos;
    if (writeCharacteristicFound && looksLikeNobra) {
        Nobra *nobraDevice = new Nobra(deviceName, client, writeChar);
        return (BluetoothDriver::Device *)nobraDevice;
    }
}
