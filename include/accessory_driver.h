#ifndef __edge_o_matic__accessory_port_h
#define __edge_o_matic__accessory_port_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

void accessory_driver_init(void);
void accessory_driver_broadcast_speed(uint8_t speed);
void accessory_driver_broadcast_arousal(uint16_t arousal);

#ifdef __cplusplus
}
#endif

#endif
