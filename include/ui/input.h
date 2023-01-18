#ifndef __ui__input_h
#define __ui__input_h

#ifdef __cplusplus
extern "C" {
#endif

#define UI_INPUT_TITLE_MAX 64

typedef struct ui_input {
    char title[UI_INPUT_TITLE_MAX];
} ui_input_t;

#ifdef __cplusplus
}
#endif

#endif
