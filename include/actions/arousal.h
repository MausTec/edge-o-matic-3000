#ifndef __actions__arousal_h
#define __actions__arousal_h

#ifdef __cplusplus
extern "C" {
#endif

#include "mt_actions.h"

/**
 * @brief Register pull-based arousal/pressure read functions and threshold
 * read/write (get_arousal, get_pressure, get_pressure_average,
 * get_arousal_threshold, set_arousal_threshold, increment_arousal_threshold).
 */
void action_arousal_init(void);

#ifdef __cplusplus
}
#endif

#endif
