#include "BluetoothDriver.h"
#include "drivers/Lovense.h"

static const char *TAG = "BluetoothDriver";

namespace BluetoothDriver {
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

    // Here's your logic to detect what device you just found. GL;HF
    BluetoothDriver::Device *buildDevice(NimBLEAdvertisedDevice *device) {
        NimBLEClient *client = nullptr;

        try {
            client = NimBLEDevice::createClient();
        } catch (...) {
            ESP_LOGE(TAG, "Failed to create client.");
            
            if (client != nullptr) {
                client->disconnect();
                NimBLEDevice::deleteClient(client);
            }

            return nullptr;
        }

        client->connect(device);

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

        std::vector<NimBLERemoteCharacteristic*> *chars = service->getCharacteristics(true);


        // Detect Lovense
        { 
            NimBLERemoteCharacteristic *rxChar = nullptr;
            NimBLERemoteCharacteristic *txChar = nullptr;

            for (NimBLERemoteCharacteristic *c : *chars) {
                if (c->canNotify()) {
                    rxChar = c;
                }

                if (c->canWrite()) {
                    txChar = c;
                }
            }

            if (txChar != nullptr && rxChar != nullptr) {
                // found two chars, tx & rx, and we can write.
                Lovense *d = new Lovense(device->getName().c_str(), client, rxChar, txChar);
                return (BluetoothDriver::Device*) d;
            }
        }
        

        return nullptr;
    }
}