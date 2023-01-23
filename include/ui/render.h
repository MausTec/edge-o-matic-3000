#ifndef __ui__render_h
#define __ui__render_h

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ui_render_flag {
    // Event processed but does not require rerender.
    NORENDER,
    // Event processed AND requires a rerender.
    RENDER,
    // Event was not processed and passes up to the caller.
    PASS
} ui_render_flag_t;

#ifdef __cplusplus
}
#endif

#endif
