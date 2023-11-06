#ifndef __AccessoryDriver_h
#define __AccessoryDriver_h

#include <stdint.h>
#include <string.h>

namespace AccessoryDriver {
    enum ProtocolFlags {
        PROTOCOL_TSCODE   = (1 << 0),
        PROTOCOL_RAW_DATA = (1 << 1),
    };

    struct RawAddressSet {
        uint8_t speed;
        uint8_t arousal;
    };

    struct Device {
        char display_name[40 + 1];
        ProtocolFlags protocol_flags;
        uint8_t address;
        RawAddressSet *registers;

        
        Device(const char *name, uint8_t addr, ProtocolFlags flags) {
            strlcpy(display_name, name, 40);
            address = addr;
            protocol_flags = flags;
        };
    };

    enum SyncMode {
        SYNC_SPEED_FOLLOWER,
        SYNC_DATA_FOLLOWER,
    };

    namespace {
        struct DeviceNode {
            Device *device;
            DeviceNode *next;
            SyncMode mode;
        };

        DeviceNode *first;
    }

    void registerDevice(Device *device, SyncMode sync_mode = SYNC_SPEED_FOLLOWER);

    void broadcastSpeed(uint8_t speed);
    void broadcastArousal(uint16_t arousal);
}

#endif