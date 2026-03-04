#include <unity.h>

static int add(int a, int b) {
    return a + b;
}

void setUp(void) {
}
void tearDown(void) {
}

void test_add(void) {
    TEST_ASSERT_EQUAL_INT(3, add(1, 2));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_add);
    return UNITY_END();
}
