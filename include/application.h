#ifndef __edge_o_matic__application_h
#define __edge_o_matic__application_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "my_basic.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define APP_EXTENSION ".mpk"
#define APP_TITLE_MAXLEN 20
#define APP_MIN_STACK (1024 * 2)

typedef enum {
    APP_OK,
    APP_FILE_NOT_FOUND,
    APP_FILE_INVALID,
    APP_NO_ENTRYPOINT,
    APP_START_NO_MEMORY,
    APP_NOT_LOADED,
} app_err_t;

typedef struct {
    char title[APP_TITLE_MAXLEN];
    char pack_path[PATH_MAX];
    char entrypoint[APP_TITLE_MAXLEN];
    struct mb_interpreter_t *interpreter;
    TaskHandle_t task;
    uint32_t stack_depth;
    app_err_t status;
} application_t;

app_err_t application_load(const char* filename, application_t **app);
app_err_t application_start(application_t *app);
app_err_t application_kill(application_t *app);
void app_dispose(application_t *app);

void application_interpreter_hooks(struct mb_interpreter_t *interpreter);

#ifdef __cplusplus
}
#endif

#endif
