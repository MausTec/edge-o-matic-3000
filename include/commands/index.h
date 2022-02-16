#ifndef __index_h
#define __index_h

#ifdef __cplusplus
extern "C" {
#endif

void commands_register_fsutils(void);

inline void commands_register_all(void) {
    commands_register_fsutils();
}

#ifdef __cplusplus
}
#endif

#endif
