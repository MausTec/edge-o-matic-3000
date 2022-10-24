#ifndef __drivers_Nobra_h
#define __drivers_Nobra_h

#include "drivers/Device.h"
#include <NimBLEDevice.h>

#define CMD_MAX_LEN 40
#define WAIT_FOR_NOTIFY_TIMEOUT_MS 1000

namespace BluetoothDriver {
    class Nobra : public Device {
    public:
        Nobra(const char *name, NimBLEClient *client, NimBLERemoteCharacteristic *remote) : 
            Device(name, client), remote(remote) {};

        static Device* detect(NimBLEAdvertisedDevice *device, NimBLEClient *client, NimBLERemoteService *service);

        bool setSpeed(uint8_t speed) override;

    protected:
        char getSpeed(uint8_t speed);
        bool send(const char *cmd);
        bool sendf(const char *fmt, ...);
        
    private:
        NimBLERemoteCharacteristic *remote;
        char lastSentSpeed;
    };
}

#endif