#ifndef __RunningAverage_h
#define __RunningAverage_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef struct running_average {
    uint16_t* buffer;
    size_t window;
    size_t index;
    uint16_t value;
    uint16_t sum;
    uint16_t averaged;
} running_average_t;

void running_average_init(running_average_t** ra, size_t window_size);
void running_average_add_value(running_average_t* ra, uint16_t value);
uint16_t running_avergae_get_average(running_average_t* ra);
void running_average_dispose(running_average_t* ra);

#ifdef __cplusplus
}
#endif

#endif