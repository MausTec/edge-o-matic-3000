#include "AccessoryDriver.h"
#include "tscode.h"
#include "eom-hal.h"
#include "esp_log.h"
#include "stdlib.h"

static const char *TAG = "AccessoryDriver";

namespace AccessoryDriver {
    static DeviceNode *first_device = nullptr;

    void registerDevice(Device *device, SyncMode sync_mode) {
        DeviceNode *node = new DeviceNode();
        node->device = new Device(*device);
        node->next = nullptr;
        node->mode = sync_mode;

        ESP_LOGE(TAG, "Registering Device: %s <0x%02x>", node->device->display_name, node->device->address);

        DeviceNode **head = &first_device;
        while (*head != nullptr) head = &(*head)->next;
        *head = node;
    }

    void broadcastSpeed(uint8_t speed) {
        DeviceNode *head = first_device;
        ESP_LOGD(TAG, "Broadcasting Speed: %d", speed);

        while (head != nullptr) {
            if (head->mode == SYNC_SPEED_FOLLOWER) {
                Device *device = head->device;
                ESP_LOGD(TAG, " -> Checking Device at 0x%02x", device->address);
                
                // TODO: Support non-tscode devices and also don't just hardcode a command actually 
                //       scan for capabilities and pick one. or many.
                if (device != nullptr && device->protocol_flags & PROTOCOL_TSCODE) {
                    tscode_command cmd = TSCODE_COMMAND_DEFAULT;
                    cmd.type = speed > 0 ? TSCODE_RECIPROCATING_MOVE : TSCODE_CONDITIONAL_STOP;
                    cmd.speed = (tscode_unit_t *) malloc(sizeof(tscode_unit_t));
                    cmd.speed->unit = TSCODE_UNIT_BYTE;
                    cmd.speed->value = speed;
                    ESP_LOGD(TAG, " -> TSCODE Device: %s", device->display_name);

                    char buffer[120] = "";
                    tscode_serialize_command(buffer, &cmd, 120);

                    ESP_LOGE(TAG, "    -> Serialized: %s", buffer);
                    tscode_dispose_command(&cmd);

                    eom_hal_accessory_master_write_str(device->address, buffer);
                }
            }

            head = head->next;
        }
    }
}