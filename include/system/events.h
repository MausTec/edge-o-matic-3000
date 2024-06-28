#ifndef __system__events_h
#define __system__events_h

#define EVENTS(X)                                                                                  \
    X(EVT_MODE_SET);                                                                               \
    X(EVT_SPEED_CHANGE);                                                                           \
    X(EVT_AROUSAL_CHANGE);                                                                         \
    X(EVT_ORGASM_DENIAL);                                                                          \
    X(EVT_ORGASM_START);                                                                           \
    X(EVT_ORGASM_CONTROL_SESSION_SETUP);                                                           \
    X(EVT_ORGASM_CONTROL_SESSION_START);                                                           \
    X(EVT_ORGASM_CONTROL_ORGASM_TRIGGER_SET);                                                      \
    X(EVT_ORGASM_CONTROL_POST_ORGASM_SET);                                                         \
    X(EVT_ORGASM_CONTROL_IS_ORGASM_PERMITED);                                                      \
    X(EVT_ORGASM_CONTROL_ORGASM_IS_PERMITED);                                                      \
    X(EVT_ORGASM_CONTROL_POST_ORGASM_START);                                                       \
    X(EVT_ORGASM_CONTROL_SHUTDOWN);

#endif
