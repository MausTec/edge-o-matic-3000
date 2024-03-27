#include "eom-hal.h"
#include "esp_log.h"
#include "maus_bus.h"
#include "system/event_manager.h"
#include "tscode.h"
#include <string.h>

static const char* TAG = "accessory_driver";

static maus_bus_err_t
_accessory_read(uint8_t address, uint8_t subaddress, uint8_t* data, size_t len) {
    maus_bus_err_t err = MAUS_BUS_OK;
    eom_hal_accessory_master_read_register(address, subaddress, data, len);
    return err;
}

static maus_bus_err_t
_accessory_write(uint8_t address, uint8_t subaddress, uint8_t* data, size_t len) {
    maus_bus_err_t err = MAUS_BUS_OK;
    eom_hal_accessory_master_write_register(address, subaddress, data, len);
    return err;
}

static void _device_speed_cb(
    maus_bus_driver_t* driver, maus_bus_device_t* device, maus_bus_address_t address, void* ptr
) {
    uint8_t speed = *((uint8_t*)ptr);

    if (driver->uart != NULL && driver->uart->transmit != NULL) {
        if (device->features.tscode) {
            ESP_LOGI(TAG, "Transmitting a TS-code speed command.");

            tscode_command_t cmd = TSCODE_COMMAND_DEFAULT;
            cmd.type = speed > 0 ? TSCODE_RECIPROCATING_MOVE : TSCODE_CONDITIONAL_STOP;
            cmd.speed = (tscode_unit_t*)malloc(sizeof(tscode_unit_t));
            cmd.speed->unit = TSCODE_UNIT_BYTE;
            cmd.speed->value = speed;
            ESP_LOGI(TAG, " -> TSCODE Device: %s", device->product_name);

            char buffer[120] = "";
            tscode_serialize_command(buffer, &cmd, 120);

            ESP_LOGI(TAG, "    -> Serialized: %s", buffer);
            tscode_dispose_command(&cmd);

            driver->uart->transmit((uint8_t*)buffer, strlen(buffer));
        }
    }
}

static void _evt_speed_change(
    const char* event,
    EVENT_HANDLER_ARG_TYPE event_arg_ptr,
    int speed,
    EVENT_HANDLER_ARG_TYPE handler_arg
) {
    static uint8_t last_value = 0;
    if (speed == last_value) return;
    last_value = speed;

    ESP_LOGD(TAG, "_evt_speed_change(%d);", speed);

    maus_bus_enumerate_devices(_device_speed_cb, &last_value);
}

static void _evt_arousal_change(
    const char* event,
    EVENT_HANDLER_ARG_TYPE event_arg_ptr,
    int arousal,
    EVENT_HANDLER_ARG_TYPE handler_arg
) {
    static uint16_t last_value = 0;
    if (arousal == last_value) return;
    last_value = arousal;

    ESP_LOGD(TAG, "_evt_arousal_change(%d);", arousal);
}

void accessory_driver_init(void) {
    maus_bus_config_t config = {
        .read = _accessory_read,
        .write = _accessory_write,
    };

    maus_bus_init(&config);

    event_manager_register_handler(EVT_SPEED_CHANGE, _evt_speed_change, NULL);
    event_manager_register_handler(EVT_AROUSAL_CHANGE, _evt_arousal_change, NULL);
}