#include "accessory/driver.h"
#include "eom-hal.h"
#include <stdlib.h>
#include <esp_log.h>

static const char *TAG = "accessory_driver";

static struct _device_scan_node {
    maus_bus_device_t device;
    struct _device_scan_node *next;
} *_scan_list;

size_t accessory_scan_bus_quick(accessory_scan_callback_t cb, void *ptr) {
    size_t count = 0;
    // TODO - scan hubs. This will be a recursive case.

    // Scan EEPROM ID chips:
    static const uint8_t EEPROM_IDS[] = { 0x50 };

    for (size_t i = 0; i < sizeof(EEPROM_IDS); i++) {
        uint8_t address = EEPROM_IDS[i];
        uint16_t guard = 0x0000;
        eom_hal_accessory_master_read_register(address, 0x00, &guard, 2);
        
        ESP_LOGD(TAG, "Contemplating device 0x%02X, guard: 0x%04X", EEPROM_IDS[i], guard);

        if (guard == 0xCAFE) {
            struct _device_scan_node *node = malloc(sizeof(struct _device_scan_node));
            if (node == NULL) return count;

            node->next = NULL;

            eom_hal_accessory_master_read_register(address, 0x00, (uint8_t*) &node->device, sizeof(maus_bus_device_t));
            ESP_LOGD(TAG, "Read in device struct, guard: 0x%04X", node->device.__guard);
            if (node->device.__guard != guard) return count;

            // Clean up data:
            node->device.vendor_name[23] = '\0';
            node->device.product_name[23] = '\0';

            if (_scan_list == NULL) {
                _scan_list = node;
            } else {
                struct _device_scan_node *p = _scan_list;
                while (p->next != NULL) p = p->next;
                p->next = node;
            }

            if (cb != NULL) {
                (*cb)(&node->device, address, ptr);
            }

            count++;
        }
    }

    return count;
}

size_t accessory_scan_bus_full(accessory_scan_callback_t cb, void *ptr) {
    size_t count = accessory_scan_bus_quick(cb, ptr);
    return count;
}

void accessory_free_device_scan(void) {
    struct _device_scan_node *p = _scan_list;

    while (p != NULL) {
        _scan_list = p->next;
        free(p);
        p = _scan_list;
    }
}