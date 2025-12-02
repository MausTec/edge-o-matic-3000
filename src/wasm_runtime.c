#include "wasm_runtime.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "wasm_export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* TAG = "wasm_runtime";

#define WASM_EXECUTOR_STACK_SIZE (8 * 1024)
#define WASM_EXECUTOR_PRIORITY (tskIDLE_PRIORITY + 2)
#define WASM_EXECUTOR_QUEUE_SIZE 16

#define WASM_INSTANCE_STACK_SIZE (16 * 1024)

// Note: Using system allocator for WAMR; no global heap buffer
static uint8_t* wasm_heap_buffer = NULL;

// Dynamic buffers for FreeRTOS executor task (allocated at init)
static StackType_t* executor_task_stack = NULL;
static StaticTask_t* executor_task_tcb = NULL;

static TaskHandle_t executor_task_handle = NULL;
static QueueHandle_t executor_queue = NULL;
static bool runtime_initialized = false;

static void wasm_executor_task(void* arg) {
    wasm_work_item_t work;

    ESP_LOGI(TAG, "WASM executor task started");

    while (true) {
        // Wait for work items (blocking)
        if (xQueueReceive(executor_queue, &work, portMAX_DELAY) == pdTRUE) {
            if (work.module_inst == NULL || work.exec_env == NULL) {
                ESP_LOGE(TAG, "Invalid work item: NULL module or exec_env");
                continue;
            }

            switch (work.type) {
            case WASM_WORK_EVENT_CALLBACK: {
                // Call on_event(topic_id, data_ptr, len)
                wasm_function_inst_t on_event =
                    wasm_runtime_lookup_function(work.module_inst, "on_event", NULL);

                if (on_event) {
                    // Allocate data in WASM linear memory
                    uint32_t data_offset = 0;
                    if (work.payload.event.data_len > 0) {
                        data_offset = wasm_runtime_module_malloc(
                            work.module_inst, work.payload.event.data_len, NULL
                        );

                        if (data_offset != 0) {
                            uint8_t* wasm_data =
                                wasm_runtime_addr_app_to_native(work.module_inst, data_offset);

                            if (wasm_data) {
                                memcpy(
                                    wasm_data, work.payload.event.data, work.payload.event.data_len
                                );
                            }
                        }
                    }

                    // Prepare arguments: on_event(topic_id, data_ptr, len)
                    uint32_t argv[3] = { work.payload.event.topic_id,
                                         data_offset,
                                         work.payload.event.data_len };

                    if (!wasm_runtime_call_wasm(work.exec_env, on_event, 3, argv)) {
                        const char* exception = wasm_runtime_get_exception(work.module_inst);
                        ESP_LOGE(
                            TAG, "on_event() exception: %s", exception ? exception : "unknown"
                        );
                    }

                    if (data_offset != 0) {
                        wasm_runtime_module_free(work.module_inst, data_offset);
                    }
                } else {
                    ESP_LOGW(TAG, "Plugin does not export on_event()");
                }
                break;
            }

            case WASM_WORK_FUNCTION_CALL: {
                if (work.payload.call.function) {
                    if (!wasm_runtime_call_wasm(
                            work.exec_env,
                            work.payload.call.function,
                            work.payload.call.argc,
                            work.payload.call.argv
                        )) {
                        const char* exception = wasm_runtime_get_exception(work.module_inst);
                        ESP_LOGE(
                            TAG, "Function call exception: %s", exception ? exception : "unknown"
                        );
                    }
                }
                break;
            }

            default: ESP_LOGE(TAG, "Unknown work item type: %d", work.type); break;
            }
        }
    }
}

esp_err_t eom_wasm_runtime_init(void) {
    if (runtime_initialized) {
        ESP_LOGW(TAG, "WASM runtime already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing WASM runtime");
    ESP_LOGI(TAG, "  Allocator: system malloc");
    ESP_LOGI(TAG, "  Instance stack: %d KB", WASM_INSTANCE_STACK_SIZE / 1024);
    ESP_LOGI(TAG, "  Executor queue: %d items", WASM_EXECUTOR_QUEUE_SIZE);
    ESP_LOGI(TAG, "  Free heap before alloc: %d bytes", esp_get_free_heap_size());

    // Allocate executor task stack
    executor_task_stack = heap_caps_malloc(WASM_EXECUTOR_STACK_SIZE, MALLOC_CAP_8BIT);
    if (!executor_task_stack) {
        ESP_LOGE(TAG, "Failed to allocate executor task stack");
        return ESP_ERR_NO_MEM;
    }

    // Allocate executor task TCB
    executor_task_tcb = heap_caps_malloc(sizeof(StaticTask_t), MALLOC_CAP_8BIT);
    if (!executor_task_tcb) {
        ESP_LOGE(TAG, "Failed to allocate executor task TCB");
        free(executor_task_stack);
        executor_task_stack = NULL;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "  WASM buffers allocated, free heap: %d bytes", esp_get_free_heap_size());

    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    init_args.mem_alloc_type = Alloc_With_System_Allocator;

    if (!wasm_runtime_full_init(&init_args)) {
        ESP_LOGE(TAG, "Failed to initialize WAMR runtime");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "WAMR runtime initialized successfully");

    // Create executor work queue
    executor_queue = xQueueCreate(WASM_EXECUTOR_QUEUE_SIZE, sizeof(wasm_work_item_t));
    if (executor_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create executor queue");
        wasm_runtime_destroy();
        return ESP_FAIL;
    }

    // Create executor task with dynamically allocated buffers
    executor_task_handle = xTaskCreateStatic(
        wasm_executor_task,
        "WASM_EXEC",
        WASM_EXECUTOR_STACK_SIZE / sizeof(StackType_t),
        NULL,
        WASM_EXECUTOR_PRIORITY,
        executor_task_stack,
        executor_task_tcb
    );

    if (executor_task_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create executor task");
        vQueueDelete(executor_queue);
        executor_queue = NULL;
        wasm_runtime_destroy();
        return ESP_FAIL;
    }

    runtime_initialized = true;
    ESP_LOGI(
        TAG,
        "WASM executor task created (stack: %d bytes, priority: %d)",
        WASM_EXECUTOR_STACK_SIZE,
        WASM_EXECUTOR_PRIORITY
    );

    return ESP_OK;
}

void eom_wasm_runtime_deinit(void) {
    if (!runtime_initialized) {
        return;
    }

    ESP_LOGI(TAG, "Deinitializing WASM runtime");

    if (executor_task_handle != NULL) {
        vTaskDelete(executor_task_handle);
        executor_task_handle = NULL;
    }

    if (executor_queue != NULL) {
        vQueueDelete(executor_queue);
        executor_queue = NULL;
    }

    wasm_runtime_destroy();

    // Free dynamically allocated buffers
    if (executor_task_tcb) {
        free(executor_task_tcb);
        executor_task_tcb = NULL;
    }
    if (executor_task_stack) {
        free(executor_task_stack);
        executor_task_stack = NULL;
    }
    if (wasm_heap_buffer) {
        free(wasm_heap_buffer);
        wasm_heap_buffer = NULL;
    }

    runtime_initialized = false;
    ESP_LOGI(TAG, "WASM runtime deinitialized");
}

esp_err_t wasm_executor_enqueue(const wasm_work_item_t* work_item) {
    if (!runtime_initialized || executor_queue == NULL) {
        ESP_LOGE(TAG, "Executor not initialized");
        return ESP_FAIL;
    }

    if (work_item == NULL) {
        ESP_LOGE(TAG, "NULL work item");
        return ESP_ERR_INVALID_ARG;
    }

    if (xQueueSend(executor_queue, work_item, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Executor queue full. dropped");
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

TaskHandle_t wasm_executor_get_task_handle(void) {
    return executor_task_handle;
}

esp_err_t wasm_load_and_run(const char* path, uint32_t stack_size, uint32_t heap_size) {
    if (path == NULL || stack_size == 0) {
        ESP_LOGE(TAG, "Invalid args: path or stack_size");
        return ESP_ERR_INVALID_ARG;
    }

    // Ensure runtime is initialized (memory issues may cause wasm to not load)
    if (eom_wasm_runtime_init() != ESP_OK) {
        ESP_LOGE(TAG, "WASM runtime init failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Loading WASM module from: %s", path);

    FILE* fp = fopen(path, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "Failed to open file: %s", path);
        return ESP_FAIL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return ESP_FAIL;
    }

    long fsize = ftell(fp);
    if (fsize <= 0) {
        fclose(fp);
        return ESP_FAIL;
    }
    rewind(fp);

    uint8_t* wasm_bytes = (uint8_t*)malloc((size_t)fsize);
    if (!wasm_bytes) {
        ESP_LOGE(TAG, "malloc failed for %ld bytes", fsize);
        fclose(fp);
        return ESP_ERR_NO_MEM;
    }

    size_t readn = fread(wasm_bytes, 1, (size_t)fsize, fp);
    fclose(fp);

    if (readn != (size_t)fsize) {
        ESP_LOGE(TAG, "fread mismatch: %u/%ld", (unsigned)readn, fsize);
        free(wasm_bytes);
        return ESP_FAIL;
    }

    char error_buf[160] = { 0 };
    wasm_module_t module =
        wasm_runtime_load(wasm_bytes, (uint32_t)fsize, error_buf, sizeof(error_buf));

    if (!module) {
        ESP_LOGE(TAG, "wasm_runtime_load failed: %s", error_buf);
        free(wasm_bytes);
        return ESP_FAIL;
    }

    // The module references string data within wasm_bytes, free after unload

    if (heap_size == 0) {
        // Provide a conservative default if caller passes 0
        heap_size = 32 * 1024;
    }

    ESP_LOGI(
        TAG, "Instantiating module (stack=%u, heap=%u)", (unsigned)stack_size, (unsigned)heap_size
    );

    wasm_module_inst_t module_inst =
        wasm_runtime_instantiate(module, stack_size, heap_size, error_buf, sizeof(error_buf));

    if (!module_inst) {
        ESP_LOGE(TAG, "wasm_runtime_instantiate failed: %s", error_buf);
        wasm_runtime_unload(module);
        free(wasm_bytes);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Executing WASM main()...");

    wasm_function_inst_t main_func = wasm_runtime_lookup_function(module_inst, "main", NULL);
    if (!main_func) {
        ESP_LOGE(TAG, "main() function not found");
        wasm_runtime_deinstantiate(module_inst);
        wasm_runtime_unload(module);
        free(wasm_bytes);
        return ESP_FAIL;
    }

    wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, WASM_INSTANCE_STACK_SIZE);
    if (!exec_env) {
        ESP_LOGE(TAG, "Failed to create execution environment");
        wasm_runtime_deinstantiate(module_inst);
        wasm_runtime_unload(module);
        free(wasm_bytes);
        return ESP_FAIL;
    }

    uint32_t argv[2] = { 0, 0 }; // argc=0, argv=NULL
    if (!wasm_runtime_call_wasm(exec_env, main_func, 2, argv)) {
        const char* exception = wasm_runtime_get_exception(module_inst);
        ESP_LOGE(TAG, "WASM exception: %s", exception ? exception : "unknown");
        wasm_runtime_destroy_exec_env(exec_env);
        wasm_runtime_deinstantiate(module_inst);
        wasm_runtime_unload(module);
        free(wasm_bytes);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "WASM main() returned: %d", argv[0]);
    wasm_runtime_destroy_exec_env(exec_env);

    wasm_runtime_deinstantiate(module_inst);
    wasm_runtime_unload(module);
    free(wasm_bytes);
    return ESP_OK;
}
