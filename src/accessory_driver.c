#include "accessory_port.h"
#include "maus_bus.h"
#include "eom-hal.h"

static maus_bus_err_t _accessory_read(uint8_t address, uint8_t subaddress, uint8_t *data, size_t len) {
    maus_bus_err_t err = MAUS_BUS_OK;
    eom_hal_accessory_master_read_register(address, subaddress, data, len);
    return err;
}

static maus_bus_err_t _accessory_write(uint8_t address, uint8_t subaddress, uint8_t *data, size_t len) {
    maus_bus_err_t err = MAUS_BUS_OK;
    eom_hal_accessory_master_write_register(address, subaddress, data, len);
    return err;
}

void accessory_driver_init(void) {
    maus_bus_config_t config = {
        .read = _accessory_read,
        .write = _accessory_write,
    };

    maus_bus_init(&config);
}