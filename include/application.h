#ifndef __edge_o_matic__application_h
#define __edge_o_matic__application_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "my_basic.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define APP_TITLE_MAXLEN 20

typedef struct {
    char title[APP_TITLE_MAXLEN];
    char archive_path[255];
    struct mb_interpreter_t *interpreter;
    TaskHandle_t task;
} application_t;

typedef enum {
    APP_OK,
    APP_FILE_NOT_FOUND,
    APP_FILE_INVALID,
    APP_NO_ENTRYPOINT,
    APP_START_NO_MEMORY,
} app_err_t;

app_err_t application_load(const char* filename, application_t *app);
app_err_t application_start(application_t *app);
app_err_t application_kill(application_t *app);
void app_dispose(application_t *app);

#ifdef __cplusplus
}
#endif

#endif
