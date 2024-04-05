#ifndef __actions__index_h
#define __actions__index_h

#ifdef __cplusplus
extern "C" {
#endif

void actions_register_system(void);

static inline void actions_register_all(void) {
    actions_register_system();
}

#ifdef __cplusplus
}
#endif

#endif
