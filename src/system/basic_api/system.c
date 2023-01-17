#include "basic_api.h"
#include <stddef.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "basic:system";

static int _yield(struct mb_interpreter_t* s, void** l) {
    int result = MB_FUNC_OK;

	mb_assert(s && l);
	mb_check(mb_attempt_func_begin(s, l));
	mb_check(mb_attempt_func_end(s, l));

    fflush(stdout);
    ESP_LOGD(TAG, "Basic has yielded.");
	vTaskDelay(1);

	return result;
}

static int _delay(struct mb_interpreter_t* s, void** l) {
    int result = MB_FUNC_OK;
    int delay_ms = 0;

    mb_assert(s && l);
    mb_check(mb_attempt_open_bracket(s, l));
    mb_check(mb_pop_int(s, l, &delay_ms));
    mb_check(mb_attempt_close_bracket(s, l));

    fflush(stdout);
    ESP_LOGI(TAG, "Basic yields for %d ms", delay_ms);
    vTaskDelay(delay_ms / portTICK_PERIOD_MS);

    return result;
}

void basic_api_register_system(struct mb_interpreter_t *bas) {
    mb_begin_module(bas, "SYSTEM");

    // Register namespace here.
    mb_register_func(bas, "YIELD", _yield);
    mb_register_func(bas, "DELAY", _delay);

    mb_end_module(bas);
}