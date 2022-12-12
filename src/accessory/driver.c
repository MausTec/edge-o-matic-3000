#include "accessory/driver.h"
#include "eom-hal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <esp_log.h>

static const char *TAG = "accessory_driver";

static const uint8_t EEPROM_IDS[] = { 0x50 };
static const uint8_t IGNORE_IDS[] = { 0x51, 0x52, 0x53, 0x54 };

static struct _device_scan_node {
    accessory_address_t *address;
    maus_bus_device_t device;
    struct _device_scan_node *next;
} *_scan_list;

size_t accessory_scan_bus_quick(accessory_scan_callback_t cb, void *ptr) {
    size_t count = 0;
    // TODO - scan hubs. This will be a recursive case.

    // Scan EEPROM ID chips:

    for (size_t i = 0; i < sizeof(EEPROM_IDS); i++) {
        uint8_t address = EEPROM_IDS[i];
        uint16_t guard = 0x0000;
        eom_hal_accessory_master_read_register(address, 0x00, &guard, 2);
        
        ESP_LOGD(TAG, "Contemplating device 0x%02X, guard: 0x%04X", address, guard);

        if (guard == 0xCAFE) {
            struct _device_scan_node *node = malloc(sizeof(struct _device_scan_node));
            if (node == NULL) return count;

            accessory_address_t addr_path = (accessory_address_t) malloc(1 + 1);

            if (addr_path != NULL) {
                addr_path[0] = address;
                addr_path[1] = 0x00;
            }

            node->address = addr_path;
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
                (*cb)(&node->device, node->address, ptr);
            }

            count++;
        }
    }

    return count;
}

/**
 * @brief returns false if addresses are not the same
 * 
 * @param a 
 * @param b 
 * @return int 
 */
int accessory_addrcmp(accessory_address_t a, accessory_address_t b) {
    uint8_t *ptr_a = a;
    uint8_t *ptr_b = b;

    while (ptr_a != NULL && ptr_b != NULL) {
        if (*ptr_a != *ptr_b) return 0;
        ptr_a++;
        ptr_b++;
    }

    if (*ptr_a != *ptr_b) return 0;
    return 1;
}

size_t accessory_get_address_depth(accessory_address_t address) {
    size_t depth = 0;
    while (address[depth++] != NULL) depth++;
    return depth;
}

uint8_t accessory_get_final_address(accessory_address_t address) {
    return address[accessory_get_address_depth(address)];
}

size_t accessory_addr2str(char *str, size_t max_len, accessory_address_t address) {
    size_t depth = accessory_get_address_depth(address);
    size_t len = ((depth + 1) * 2) + depth;

    if (str != NULL && max_len > 0) {

    }

    return len;
}

int _device_idx_by_address(accessory_address_t address) {
    struct _device_scan_node *p = _scan_list;
    int i = 0;

    while (p != NULL) {
        if (accessory_addrcmp(address, p->address)) {
            return i;
        }
        p = p->next;
        i++;
    }

    return -1;
}

int _address_is_ignored(uint8_t address) {
    for (size_t ign = 0; ign < sizeof(IGNORE_IDS); ign++) {
        if (IGNORE_IDS[ign] == address) return 1;
    }
    return 0;
}

size_t accessory_scan_bus_full(accessory_scan_callback_t cb, void *ptr) {
    size_t count = accessory_scan_bus_quick(cb, ptr);
    size_t addr_depth = 0;

    for (size_t i = 0; i < 127; i++) {
        uint8_t address = i;
        uint8_t addr_tmp[] = { address, 0x00 };
        
        if (_device_idx_by_address(addr_tmp) != -1) continue;
        if (_address_is_ignored(address)) continue;

        eom_hal_err_t err = eom_hal_accessory_master_probe(address);

        if (err != EOM_HAL_OK) {
            continue;
        }

        char addr_str[20] = "";
        accessory_addr2str(addr_str, 20, address);
        ESP_LOGI(TAG, "Contemplating device 0x%02X (full scan) @ %s", address, addr_str);

        // Device can be added as an unknown:
        struct _device_scan_node *node = malloc(sizeof(struct _device_scan_node));
        if (node == NULL) return count;

        accessory_address_t addr_path = (accessory_address_t) malloc(addr_depth + 2);

        if (addr_path != NULL) {
            memcpy(addr_path, addr_tmp, addr_depth + 2);
        }

        node->address = addr_path;
        node->next = NULL;

        memset(&node->device, 0x00, sizeof(node->device));
        snprintf(node->device.vendor_name, MAUS_BUS_VENDOR_MAX_LENGTH, "<Unknown - %d>", node->device.vendor_id);
        snprintf(node->device.product_name, MAUS_BUS_PRODUCT_MAX_LENGTH, "<Unknown 0x%02X - %d>", address, node->device.product_id);

        ESP_LOGI(TAG, "Added: %s - %s", node->device.vendor_name, node->device.product_name);

        if (_scan_list == NULL) {
            _scan_list = node;
        } else {
            struct _device_scan_node *p = _scan_list;
            while (p->next != NULL) p = p->next;
            p->next = node;
        }

        if (cb != NULL) {
            (*cb)(&node->device, node->address, ptr);
        }

        count++;
    }

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