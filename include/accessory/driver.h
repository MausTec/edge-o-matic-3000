#ifndef __accessory__driver_h
#define __accessory__driver_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "maus_bus.h"

typedef enum {
    ACCESSORY_CONNECTED,
    ACCESSORY_PROBED,
    ACCESSORY_TIMEOUT,
    ACCESSORY_DISCONNECTED,
} accessory_status_t;

typedef struct {
    
} accessory_driver_t;

typedef struct {
    uint8_t address;
    maus_bus_device_t *device;
    accessory_status_t status;
    accessory_driver_t *driver;
} accessory_device_t;

typedef void (*accessory_scan_callback_t)(maus_bus_device_t* device, uint8_t address, void* ptr);

/**
 * @brief Scans the accessory bus only walking hubs and ID chips.
 * 
 * This will report any accessory identified by an ID chip, but not unknown devices. This can be
 * called when PIDET is toggled, which comes from HAL. For a manual scan to catch all devices,
 * please use accessory_scan_bus_full.
 * 
 * @param cb 
 * @return size_t Count of devices found.
 */
size_t accessory_scan_bus_quick(accessory_scan_callback_t cb, void* ptr);

/**
 * @brief Returns a full list of devices found on the bus.
 * 
 * This will report any unknown device as well as what the quick scan reports. It only ignores the
 * one internal chip that shares the bus on first-gen EOM boards.
 * 
 * @param cb 
 * @return size_t Count of devices found.
 */
size_t accessory_scan_bus_full(accessory_scan_callback_t cb, void* ptr);

/**
 * @brief Frees the internal device list generated from the last scan.
 * 
 * Be sure to call this when you're done with the scan results! All device pointers will be freed,
 * so be sure to copy those to your own structure.
 */
void accessory_free_device_scan(void);

#ifdef __cplusplus
}
#endif

#endif
