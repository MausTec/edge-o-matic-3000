#ifndef __actions__action_types_h
#define __actions__action_types_h

#ifdef __cplusplus
extern "C" {
#endif

#include "maus_bus.h"
#include "mt_actions.h"
#include <stdint.h>

/**
 * @brief EOM event constants for registratiton with mta
 */
typedef enum {
    MTA_EVT_ORGASM_DENIAL = 0,
    MTA_EVT_EDGE_START,
    MTA_EVT_AROUSAL_CHANGE,
    MTA_EVT_MODE_SET,
    MTA_EVT_SPEED_CHANGE,
    MTA_EVT_ORGASM_START,
} mta_event_id_t;

/**
 * @brief planned drivers/caps to provide more functions to the script namespace. System functioinss
 * (if/else, comparisons, etc) are always avail
 */
typedef enum {
    MTA_CAP_GPIO, // GPIO hardware functions
    MTA_CAP_UART, // UART hardware functions
    MTA_CAP_I2C,  // I2C hardware functions
} mta_capability_t;

#define MTA_MAX_CAPABILITIES 8

/**
 * @brief GPIO-specific function IDs
 */
typedef enum {
    GPIO_SET_PIN = 0,
    GPIO_GET_PIN,
    GPIO_SET_MODE,
} mta_gpio_fn_t;

/**
 * @brief UART-specific function IDs
 */
typedef enum {
    UART_WRITE = 0,
    UART_READ,
    UART_SET_MODE,
} mta_uart_fn_t;

/**
 * @brief GPIO pin configuration
 */
typedef struct {
    uint8_t pin;
    uint8_t mode;  // 0=INPUT, 1=OUTPUT
    uint8_t level; // Initial level for OUTPUT mode
} mta_gpio_pin_config_t;

/**
 * @brief GPIO capability configuration
 */
typedef struct {
    mta_gpio_pin_config_t* pins;
    uint8_t count;
} mta_gpio_config_t;

/**
 * @brief UART capability configuration
 */
typedef struct {
    uint32_t baud;
    uint8_t data_bits;
    uint8_t parity;
    uint8_t stop_bits;
} mta_uart_config_t;

/**
 * @brief eom -> mta registration data
 */
typedef struct {
    uint16_t capabilities; // Bitfield of mta_capability_t
    void* cap_configs[MTA_MAX_CAPABILITIES];
    maus_bus_device_link_t* device;
} mta_host_plugin_data_t;

void mta_init(mta_event_map_t* event_map, size_t map_size);

void mta_load_plugin(mta_plugin_t** driver, cJSON* root);
void mta_unload_plugin(mta_plugin_t* driver);

void mta_init_device(mta_plugin_t* driver, maus_bus_device_link_t* device);
void mta_deinit_device(mta_plugin_t* driver);

static inline mta_host_plugin_data_t* mta_get_host_data(mta_plugin_t* plugin) {
    return (mta_host_plugin_data_t*)plugin->device;
}

static inline maus_bus_device_link_t* mta_get_device(mta_plugin_t* plugin) {
    mta_host_plugin_data_t* host_data = mta_get_host_data(plugin);
    return host_data ? host_data->device : NULL;
}

#ifdef __cplusplus
}
#endif

#endif
