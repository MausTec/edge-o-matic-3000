#ifndef __util__i_n_h
#define __util__i_n_h

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

esp_err_t i18n_init(void);
void i18n_deinit(void);
void i18n_dump(void);
const char* _(const char* str);

#ifdef __cplusplus
}
#endif

#endif
