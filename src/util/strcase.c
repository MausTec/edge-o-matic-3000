#include "util/strcase.h"
#include <ctype.h>
#include <string.h>

size_t str_to_camel_case(char* out, size_t out_len, const char* in) {
    if (in == NULL) return -1;

    size_t len = strlen(in);
    size_t final = 0;
    int uc_next = 0;

    for (int i = 0; i < len; i++) {
        if (out != NULL && i >= out_len) break;
        if (in[i] == '_' || in[i] == '-') {
            uc_next = 1;
            continue;
        }

        if (out != NULL) {
            out[final] = uc_next ? toupper(in[i]) : tolower(in[i]);
        }

        uc_next = 0;
        final++;
    }

    if (out != NULL) {
        out[final] = '\0';
    }

    return final;
}