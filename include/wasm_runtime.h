#ifndef __wasm_runtime_h
#define __wasm_runtime_h

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "wasm_export.h"

/**
 * @brief WASM executor will either run a callback or publish an event
 */
typedef enum {
    WASM_WORK_EVENT_CALLBACK, // Call on_event(topic_id, data, len)
    WASM_WORK_FUNCTION_CALL,  // Call arbitrary exported function
} wasm_work_type_t;

/**
 * @brief Work item for WASM executor task
 */
typedef struct {
    wasm_work_type_t type;
    wasm_module_inst_t module_inst;
    wasm_exec_env_t exec_env;

    union {
        struct {
            uint32_t topic_id;
            uint8_t data[64];
            uint32_t data_len;
        } event;

        struct {
            wasm_function_inst_t function;
            uint32_t argc;
            uint32_t argv[8];
        } call;
    } payload;
} wasm_work_item_t;

/**
 * @brief Initialize WAMR runtime with light memory settings
 *
 * 48KB heap for all WASM instances
 * + 16KB stack per instance
 *
 */
esp_err_t eom_wasm_runtime_init(void);

void eom_wasm_runtime_deinit(void);

esp_err_t wasm_executor_enqueue(const wasm_work_item_t* work_item);

TaskHandle_t wasm_executor_get_task_handle(void);

/**
 * @brief Load a raw WASM/WASI module from filesystem and run its main()
 */
esp_err_t wasm_load_and_run(const char* path, uint32_t stack_size, uint32_t heap_size);

#ifdef __cplusplus
}
#endif

#endif // __wasm_runtime_h
