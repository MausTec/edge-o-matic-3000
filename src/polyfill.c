#include "polyfill.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

unsigned long millis(void) {
    return esp_timer_get_time() / 1000UL;
}

unsigned long micros(void) {
    return esp_timer_get_time();
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
    if (in_max - in_min == 0) {
        return 0;
    }
    
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void delay(long ms) {
    vTaskDelay(ms / portTICK_PERIOD_MS);
}