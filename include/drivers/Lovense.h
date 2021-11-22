#ifndef __drivers_Lovense_h
#define __drivers_Lovense_h

#include "drivers/Device.h"

#define CMD_MAX_LEN 40
#define WAIT_FOR_NOTIFY_TIMEOUT_MS 1000

namespace BluetoothDriver {
    class Lovense : public Device {
    public:
        Lovense(const char *name, NimBLEClient *client, NimBLERemoteCharacteristic *rxChar, NimBLERemoteCharacteristic *txChar) : 
            Device(name, client), rxChar(rxChar), txChar(txChar) {};

        static Device* detect(NimBLEAdvertisedDevice *device, NimBLEClient *client, NimBLERemoteService *service);

        bool setSpeed(uint8_t speed) override;

    protected:
        bool send(const char *cmd);
        bool sendf(const char *fmt, ...);
        size_t read(char *buf, size_t len, bool waitForNotify);
        
        bool waitForNotify();
        void onNotify(uint8_t *data, size_t len);

    private:
        NimBLERemoteCharacteristic *rxChar;
        NimBLERemoteCharacteristic *txChar;

        bool notifyPending = false;
    };
}

#endif