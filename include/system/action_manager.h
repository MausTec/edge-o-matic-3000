#ifndef __system__action_manager_c
#define __system__action_manager_c

#ifdef __cplusplus
extern "C" {
#endif

void action_manager_init(void);
void action_manager_load_drivercfg(const char* filename);
void action_manager_load_all_drivercfg(void);
void action_manager_dispatch_event(const char* event, int val);

#ifdef __cplusplus
}
#endif

#endif
