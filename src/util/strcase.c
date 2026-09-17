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

/**
 * @brief Converts a PascalCase string to snake_case.
 *
 * If you provide SCREAMING_SNAKE_CASE you will get back c_u_r_s_e_d_s_n_a_k_e_c_a_s_e
 *
 * @param out
 * @param out_len
 * @param in
 * @return size_t
 */
size_t str_to_snake_case(char* out, size_t out_len, const char* in) {
    if (in == NULL) return -1;

    size_t len = strlen(in);
    size_t final = 0;

    for (int i = 0; i < len; i++) {
        if (out != NULL && i >= out_len) break;
        if (isupper(in[i])) {
            if (i > 0 && in[i - 1] != '_' && in[i - 1] != '-') {
                if (out != NULL && final < out_len) {
                    out[final] = '_';
                }
                final++;
            }
            if (out != NULL && final < out_len) {
                out[final] = tolower(in[i]);
            }
        } else if (in[i] == '-' || in[i] == ' ') {
            if (out != NULL && final < out_len) {
                out[final] = '_';
            }
        } else {
            if (out != NULL && final < out_len) {
                out[final] = in[i];
            }
        }
        final++;
    }

    if (out != NULL) {
        out[final] = '\0';
    }

    return final;
}