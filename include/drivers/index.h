#ifndef __drivers__index_h
#define __drivers__index_h

#ifdef __cplusplus
extern "C" {
#endif

#include "bluetooth_driver.h"

extern const bluetooth_driver_t PLUGIN_DRIVER;

static const bluetooth_driver_t* BT_DRIVERS[] = {
    &PLUGIN_DRIVER,
};

#ifdef __cplusplus
}
#endif

#endif
