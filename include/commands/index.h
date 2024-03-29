#ifndef __index_h
#define __index_h

#ifdef __cplusplus
extern "C" {
#endif

void commands_register_fsutils(void);
void commands_register_config(void);
void commands_register_system(void);
void commands_register_edging(void);
void commands_register_bus(void);
void commands_register_script(void);

static inline void commands_register_all(void) {
    commands_register_fsutils();
    commands_register_config();
    commands_register_system();
    commands_register_edging();
    commands_register_bus();
    commands_register_script();
}

#ifdef __cplusplus
}
#endif

#endif
