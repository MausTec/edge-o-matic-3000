#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <unity.h>

void setUp(void) {
}
void tearDown(void) {
}

void test_truth(void) {
    TEST_ASSERT_TRUE(1);
}

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(100));
    UNITY_BEGIN();
    RUN_TEST(test_truth);
    UNITY_END();
}
