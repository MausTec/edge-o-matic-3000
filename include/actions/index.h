#ifndef __actions__index_h
#define __actions__index_h

#ifdef __cplusplus
extern "C" {
#endif

void actions_register_system(void);
void action_config_init(void);
void action_ble_init(void);

static inline void actions_register_all(void) {
    actions_register_system();
    action_config_init();
    action_ble_init();
}

#ifdef __cplusplus
}
#endif

#endif
