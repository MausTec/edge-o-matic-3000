#ifndef __drivers_Device_h
#define __drivers_Device_h

#include <NimBLEDevice.h>
#include <stdint.h>
#include <string.h>

namespace BluetoothDriver {
    class Device {
    public:
        Device(const char* display_name, NimBLEClient* client) {
            strlcpy(this->display_name, display_name, 40);
            this->ble_client = client;
        };

        static Device* detect(NimBLEAdvertisedDevice* device, NimBLEClient* client, NimBLERemoteService* service);

        // Override these:
        virtual bool setSpeed(uint8_t speed);
        virtual uint8_t getBatteryLevel();

        // These are common to all devices:
        void getName(char* buffer, size_t len);
        void disconnect(void);
        bool isConnected(void);
        bool reconnect(void);

    private:
        NimBLEClient* ble_client;
        char display_name[40 + 1];
    };

    
    typedef Device* (*DeviceDetectionCallback)(NimBLEAdvertisedDevice*, NimBLEClient*, NimBLERemoteService*);
}

#endif