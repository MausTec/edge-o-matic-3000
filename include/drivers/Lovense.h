#ifndef __drivers_Lovense_h
#define __drivers_Lovense_h

#include "drivers/Device.h"

namespace BluetoothDriver {
    class Lovense : public Device {
    public:
        Lovense(const char *name, NimBLEClient *client, NimBLERemoteCharacteristic *rxChar, NimBLERemoteCharacteristic *txChar) : 
            Device(name, client), rxChar(rxChar), txChar(txChar) {};

        void setSpeed(uint8_t speed) override;

    protected:
        bool sendRawCmd(const char *cmd);
        size_t readRaw(char *buf, size_t len, bool waitForNotify);
        bool waitForNotify();

    private:
        NimBLERemoteCharacteristic *rxChar;
        NimBLERemoteCharacteristic *txChar;
    };
}

#endif