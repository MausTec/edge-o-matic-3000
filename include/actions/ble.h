#ifndef __actions__ble_h
#define __actions__ble_h

#ifdef __cplusplus
extern "C" {
#endif

#include "mt_actions.h"

/**
 * @brief Register BLE host functions (bleWrite, bleWriteNoResponse)
 *
 * These functions allow mt-actions plugins to send BLE GATT writes
 * through the device context attached to the plugin.
 */
void action_ble_init(void);

#ifdef __cplusplus
}
#endif

#endif
