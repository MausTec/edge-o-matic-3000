#ifndef __polyfill_h
#define __polyfill_h

#include "esp_timer.h"

#ifndef __cplusplus
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

typedef uint8_t byte;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @deprecated Use esp_timer_get_time() / 1000UL;
 */
unsigned long millis(void);

/**
 * @deprecated Use esp_timer_get_time();
 */
unsigned long micros(void);

/**
 * @deprecated Or not? Maybe.
 */
long map(long x, long in_min, long in_max, long out_min, long out_max);

/**
 * @deprecated Use vTaskDelay();
 */
void delay(long ms);

#ifdef __cplusplus
}
#endif

#endif
