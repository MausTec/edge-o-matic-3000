#ifndef __api_index_h
#define __api_index_h

#ifdef __cplusplus
extern "C" {
#endif

void api_register_fsutils(void);
void api_register_config(void);
void api_register_system(void);
void api_register_edging(void);

static inline void api_register_all(void) {
    api_register_fsutils();
    api_register_config();
    api_register_system();
    api_register_edging();
}

#ifdef __cplusplus
}
#endif

#endif
