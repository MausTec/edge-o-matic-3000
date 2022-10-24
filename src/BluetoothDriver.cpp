#include "BluetoothDriver.h"
#include "drivers/Lovense.h"
#include "drivers/Nobra.h"

static const char *TAG = "BluetoothDriver";

namespace BluetoothDriver {
    /**
     * When adding device drivers, be sure to update this list with the 
     * driver's detect routine. Thanks!
     */
    static const DeviceDetectionCallback DEVICE_DRIVERS[] = {
        Nobra::detect,
        Lovense::detect
    };

    void registerDevice(Device *device) {
        unregisterDevice(device);
        DeviceNode *node = new DeviceNode();
        node->device = device;
        node->next = nullptr;
        
        DeviceNode **head = &first;
        while (*head != nullptr) head = &(*head)->next;
        *head = node;
    }

    void unregisterDevice(Device *device) {
        DeviceNode *node = first;
        DeviceNode *prev = nullptr;

        while (node != nullptr) {
            if (node->device == device) {
                if (prev == nullptr) {
                    first = node->next;
                } else {
                    prev->next = node->next;
                }

                delete device;
                delete node;
                return;
            }

            prev = node;
            node = node->next;
        }
    }

    void broadcastSpeed(uint8_t speed) {
        DeviceNode *head = first;

        while (head != nullptr) {
            BluetoothDriver::Device *device = head->device;

            if (device != nullptr) {
                device->setSpeed(speed);
            }

            head = head->next;
        }
    }

    BluetoothDriver::Device *getDevice(size_t n) {
        DeviceNode *head = first;
        while (head != nullptr && --n > 0) {
            head = head->next;
        }
        
        if (head == nullptr)
            return nullptr;

        return head->device;
    }

    size_t getDeviceCount(void) {
        DeviceNode *head = first;
        size_t count = 0;
        while (head != nullptr) {
            count++;
            head = head->next;
        }
        return count;
    }

    BluetoothDriver::Device *buildDevice(NimBLEAdvertisedDevice *device) {
        NimBLEClient *client = nullptr;

        client = NimBLEDevice::createClient();
        
        if (client == nullptr) {
            ESP_LOGE(TAG, "Failed to create client.");
            
            if (client != nullptr) {
                client->disconnect();
                NimBLEDevice::deleteClient(client);
            }

            return nullptr;
        }

        if (!client->connect(device)) {
            ESP_LOGE(TAG, "Failed to connect to device (%s).", device->toString().c_str());
            return nullptr;
        }
    
        BLEUUID serviceUUID = device->getServiceUUID();
        if (serviceUUID.toString() == "") {
            ESP_LOGE(TAG, "No serviceUUID advertised.");
            
            if (client != nullptr) {
                client->disconnect();
                NimBLEDevice::deleteClient(client);
            }

            return nullptr;
        }

        NimBLERemoteService *service = client->getService(serviceUUID);
        if (service == nullptr) {
            ESP_LOGE(TAG, "Failed to find service: %s", serviceUUID.toString().c_str());
            
            if (client != nullptr) {
                client->disconnect();
                NimBLEDevice::deleteClient(client);
            }

            return nullptr;
        }

        Device *detected_device = nullptr;

        for (size_t i = 0; i < sizeof(DEVICE_DRIVERS); i += sizeof(DeviceDetectionCallback)) {
            detected_device = DEVICE_DRIVERS[i](device, client, service);
            if (detected_device != nullptr) break;
        }

        return detected_device;
    }
}