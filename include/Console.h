#ifndef __console_h
#define __console_h

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_vfs.h"
#include <stdio.h>
#include <string.h>

enum command_err {
    CMD_FAIL = -1,
    CMD_OK = 0,
    CMD_ARG_ERR = 1,
    CMD_SUBCOMMAND_NOT_FOUND = 253,
    CMD_SUBCOMMAND_REQUIRED = 254,
    CMD_NOT_FOUND = 255,
};

typedef enum command_err command_err_t;

struct console {
    char cwd[PATH_MAX + 1];
    FILE* out;
    command_err_t err;
};

typedef struct console console_t;

typedef command_err_t (*command_func_t)(int argc, char** argv, console_t* console);

struct command {
    const char* command;
    const char* help;
    const char alias;
    command_func_t func;
    const struct command* subcommands[];
};

typedef struct command command_t;

void console_init(void);
void console_ready(void);
void console_handle_message(char* line, char* out, size_t len);
void console_send_file(const char* filename, console_t* console);
void console_register_command(const command_t* command);
void console_run_command(int argc, char** argv, console_t* console);
int console_cd(const char* path);

#ifdef __cplusplus
}
#endif

#endif
