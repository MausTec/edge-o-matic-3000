#include "wasm_runtime.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "wasm_export.h"
#include <string.h>

static const char* TAG = "wasm_runtime";

#define WASM_EXECUTOR_STACK_SIZE (8 * 1024)
#define WASM_EXECUTOR_PRIORITY (tskIDLE_PRIORITY + 2)
#define WASM_EXECUTOR_QUEUE_SIZE 16

#define WASM_GLOBAL_HEAP_SIZE (48 * 1024)
#define WASM_INSTANCE_STACK_SIZE (16 * 1024)

// Dynamic heap buffer for WASM runtime (allocated at init)
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
    ESP_LOGI(TAG, "  Global heap: %d KB", WASM_GLOBAL_HEAP_SIZE / 1024);
    ESP_LOGI(TAG, "  Instance stack: %d KB", WASM_INSTANCE_STACK_SIZE / 1024);
    ESP_LOGI(TAG, "  Executor queue: %d items", WASM_EXECUTOR_QUEUE_SIZE);
    ESP_LOGI(TAG, "  Free heap before alloc: %d bytes", esp_get_free_heap_size());

    // Allocate WASM heap buffer (aligned for WAMR)
    wasm_heap_buffer = heap_caps_aligned_alloc(8, WASM_GLOBAL_HEAP_SIZE, MALLOC_CAP_8BIT);
    if (!wasm_heap_buffer) {
        ESP_LOGE(TAG, "Failed to allocate WASM heap buffer (%d bytes)", WASM_GLOBAL_HEAP_SIZE);
        return ESP_ERR_NO_MEM;
    }

    // Allocate executor task stack
    executor_task_stack = heap_caps_malloc(WASM_EXECUTOR_STACK_SIZE, MALLOC_CAP_8BIT);
    if (!executor_task_stack) {
        ESP_LOGE(TAG, "Failed to allocate executor task stack");
        free(wasm_heap_buffer);
        wasm_heap_buffer = NULL;
        return ESP_ERR_NO_MEM;
    }

    // Allocate executor task TCB
    executor_task_tcb = heap_caps_malloc(sizeof(StaticTask_t), MALLOC_CAP_8BIT);
    if (!executor_task_tcb) {
        ESP_LOGE(TAG, "Failed to allocate executor task TCB");
        free(executor_task_stack);
        free(wasm_heap_buffer);
        executor_task_stack = NULL;
        wasm_heap_buffer = NULL;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "  WASM buffers allocated, free heap: %d bytes", esp_get_free_heap_size());

    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = wasm_heap_buffer;
    init_args.mem_alloc_option.pool.heap_size = WASM_GLOBAL_HEAP_SIZE;

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
