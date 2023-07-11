#ifndef __drivers__index_h
#define __drivers__index_h

#ifdef __cplusplus
extern "C" {
#endif

#include "bluetooth_driver.h"

extern const bluetooth_driver_t LOVENSE_DRIVER;

static const bluetooth_driver_t* BT_DRIVERS[] = {
    &LOVENSE_DRIVER,
};

#ifdef __cplusplus
}
#endif

#endif
