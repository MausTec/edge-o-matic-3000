#ifndef __util__strcase_h
#define __util__strcase_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

size_t str_to_camel_case(char* out, size_t out_len, const char* in);

#ifdef __cplusplus
}
#endif

#endif
