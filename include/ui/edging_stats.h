#ifndef __ui__edging_stats_h
#define __ui__edging_stats_h

#ifdef __cplusplus
extern "C" {
#endif

#include "system/event_manager.h"

volatile static struct {
    uint16_t arousal_peak;
    uint64_t arousal_peak_last_ms;
    uint64_t arousal_peak_update_ms;
    uint64_t arousal_change_notice_ms;
    uint8_t denial_count;
    event_handler_node_t* _h_denial;
} state = { 0 };

#ifdef __cplusplus
}
#endif

#endif
