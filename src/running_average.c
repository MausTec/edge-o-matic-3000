#include "running_average.h"
#include <stdlib.h>

void running_average_init(running_average_t** ra, size_t window_size) {
    running_average_t* init = (running_average_t*)malloc(sizeof(running_average_t));
    if (init == NULL)
        return;
    init->buffer = (uint16_t*)malloc(sizeof(uint16_t) * window_size);
}

void running_average_add_value(running_average_t* ra, uint16_t value) {
    ra->sum -= ra->buffer[ra->index];
    ra->buffer[ra->index] = value;
    ra->sum = ra->sum + value;
    ra->index = (ra->index + 1) % ra->window;
    ra->averaged = ra->sum / ra->window;
}

uint16_t running_avergae_get_average(running_average_t* ra) { return ra->averaged; }

void running_average_dispose(running_average_t* ra) {
    if (ra == NULL)
        return;
    free(ra->buffer);
    free(ra);
}