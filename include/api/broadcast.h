#ifndef __broadcast_h
#define __broadcast_h

#ifdef __cplusplus
extern "C" {
#endif

void api_broadcast_config(void);
void api_broadcast_readings(void);
void api_broadcast_storage_status(void);
void api_broadcast_network_status(void);

#ifdef __cplusplus
}
#endif

#endif
