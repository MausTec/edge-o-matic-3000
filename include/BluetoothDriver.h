#ifndef __BluetoothDriver_h
#define __BluetoothDriver_h

#include <NimBLEDevice.h>
#include "drivers/Device.h"

#include <stdint.h>
#include <string.h>

namespace BluetoothDriver {
    namespace {
        struct DeviceNode {
            Device *device;
            DeviceNode *next;
        };

        DeviceNode *first;
    }

    /**
     * This should lookup the most appropriate class to use as an adapter. It'll return nullptr if there's
     * no such device adapter to be used. Remember to free.
     * 
     * If you're adding new devices, update this implementation to find them.
     */
    Device *buildDevice(NimBLEAdvertisedDevice *device);

    void registerDevice(Device *device);
    void unregisterDevice(Device *device);

    void broadcastSpeed(uint8_t speed);
    Device *getDevice(size_t n);
    size_t getDeviceCount(void);
}

#endif