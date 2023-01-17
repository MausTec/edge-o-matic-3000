#include "console.h"

#include "SDHelper.h"

#include "config.h"
#include "config_defs.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "VERSION.h"
#include "commands/index.h"
#include "driver/uart.h"
#include "eom-hal.h"
#include "eom_tscode_handler.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "linenoise/linenoise.h"
#include "tscode.h"

#include "my_basic.h"

#define PROMPT "eom:%s> "
#define ARGV_MAX 16
#define CMDLINE_MAX 256

static const char* TAG = "console";
static esp_console_repl_t* _repl = NULL;
static char _history_file[SD_MAX_DIR_LENGTH + 1] = {0};
static bool _tscode_mode = false;

static struct cmd_node {
    command_t* command;
    struct cmd_node* next;
}* _first = NULL;

static command_err_t cmd_help(int argc, char** argv, console_t* console) {
    struct cmd_node* ptr = _first;

    if (argc == 0) {
        fprintf(console->out, "Edge-o-Matic 3000, " VERSION "\n\nCOMMANDS\n");
    }

    while (ptr != NULL) {
        if (argc == 0) {
            fprintf(console->out, "    %-10s  %s\n", ptr->command->command, ptr->command->help);
        } else if (!strcasecmp(argv[0], ptr->command->command)) {
            fprintf(console->out, "%s\n", ptr->command->help);

            if (ptr->command->subcommands[0] != NULL) {
                fprintf(console->out, "\nSUBCOMMANDS\n");
                size_t idx = 0;
                command_t* sub = NULL;

                while ((sub = ptr->command->subcommands[idx++]) != NULL) {
                    fprintf(console->out, "  %-10s  %s\n", sub->command, sub->help);
                }
            }
            return CMD_OK;
        }

        ptr = ptr->next;
    }

    if (argc != 0) {
        fprintf(console->out, "Cannot find help for command: %s\n", argv[0]);
        return CMD_FAIL;
    }

    return CMD_OK;
}

static const command_t cmd_help_s = {
    .command = "help",
    .help = "Get help about all or a specific command.",
    .alias = '?',
    .func = &cmd_help,
    .subcommands = {NULL},
};

static void register_help(void) { console_register_command(&cmd_help_s); }

static void reinitialize_console(void) {
    if (_tscode_mode || Config.console_basic_mode) {
        linenoiseSetDumbMode(true);
        linenoiseAllowEmpty(false);
        linenoiseSetMultiLine(false);
        linenoiseHistorySetMaxLen(0);
    } else {
        linenoiseAllowEmpty(true);
        linenoiseSetMultiLine(true);
        linenoiseHistorySetMaxLen(100);

        if (Config.store_command_history) {
            SDHelper_getAbsolutePath(_history_file, SD_MAX_DIR_LENGTH, "history.txt");
            linenoiseHistoryLoad(_history_file);
        }

        int resp = linenoiseProbe();
        if (resp != 0) {
            printf("Your terminal does not support escape sequences. This experience may\n"
                   "not be as pretty as on other consoles: %d\n",
                   resp);
            linenoiseSetDumbMode(true);
        }
    }
}

/**
 * Intercept TS-code commands to see if we're changing modes, otherwise pass to global handler.
 */
static tscode_command_response_t tscode_handler(tscode_command_t* cmd, char* response,
                                                size_t resp_len) {
    if (cmd->type == TSCODE_EXIT_TSCODE_MODE) {
        _tscode_mode = false;
        reinitialize_console();
        return TSCODE_RESPONSE_OK;
    } else {
        return eom_tscode_handler(cmd, response, resp_len);
    }
}

static void console_task(void* args);

/**
 * Dedicated system task for handling the console initialization and looping.
 */
static void console_idle_task(void* args) {

    // Delay here on console initialization to allow the user to connect a terminal first.
    if (!Config.console_basic_mode) {
        printf("Press any key to initialize the terminal.\n");
        for (;;) {
            if (getc(stdin) != EOF)
                break;
            vTaskDelay(1);
        }
    }

    xTaskCreate(&console_task, "CONSOLE", 1024*8, NULL, tskIDLE_PRIORITY, NULL);
    vTaskDelete(NULL);
}

static void console_task(void* args) {
    char *prompt = (char*) malloc(PATH_MAX + 7);

    reinitialize_console();

    console_t uart_console = {
        .out = stdout,
        .err = -1,
    };

    strncpy(uart_console.cwd, eom_hal_get_sd_mount_point(), PATH_MAX);

    for (;;) {
        char* line = NULL;

        // TScode does not use a prompt:
        if (_tscode_mode) {
            line = linenoise("");
        } else {
            snprintf(prompt, PATH_MAX + 7, PROMPT, uart_console.cwd);
            line = linenoise(prompt);
        }

        if (!line) {
            continue;
        }

        if (strcasecmp(line, "tscode") == 0) {
            _tscode_mode = true;
            reinitialize_console();
        }

        if (line[0] != '\0') {
            if (_tscode_mode) {
                char out[256] = {0};
                tscode_process_buffer(line, &tscode_handler, out, 256);
                printf("%s\n", out);
            } else {
                linenoiseHistoryAdd(line);

                if (_history_file[0] != '\0') {
                    linenoiseHistorySave(_history_file);
                }

                char* argv[ARGV_MAX] = {0};
                int argc = esp_console_split_argv(line, argv, ARGV_MAX);
                console_run_command(argc, argv, &uart_console);
            }
        }

        linenoiseFree(line);
    }

    vTaskDelete(NULL);
}

void console_init(void) {
    register_help();
    commands_register_all();
    eom_tscode_install();

    // Set UART up:
    fflush(stdout);
    fsync(fileno(stdout));
    setvbuf(stdin, NULL, _IONBF, 0);
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

    const uart_config_t uart_config = {
        .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .source_clk = UART_SCLK_REF_TICK,
    };

    // Install UART driver (again):
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config));

    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    esp_console_config_t console_config = {
        .max_cmdline_args = ARGV_MAX,
        .max_cmdline_length = CMDLINE_MAX,
    };

    ESP_ERROR_CHECK(esp_console_init(&console_config));
}

void console_register_command(const command_t* command) {
    struct cmd_node* n = (struct cmd_node*)malloc(sizeof(struct cmd_node));
    n->command = command;
    n->next = NULL;

    if (_first == NULL) {
        _first = n;
    } else {
        struct cmd_node* ptr = _first;
        while (ptr->next != NULL)
            ptr = ptr->next;
        ptr->next = n;
    }
}

void console_ready(void) {
    xTaskCreate(&console_idle_task, "con_idle", 1024*2, NULL, tskIDLE_PRIORITY, NULL);
}

void console_send_file(const char* filename, console_t* console) {
    FILE* file = fopen(filename, "rb");
    long size = -1;

    if (file) {
        fseek(file, 0, SEEK_END);
        size = ftell(file);
        rewind(file);
    }

    fprintf(console->out, ">>>HEXFILE:%ld;%s\n", size, filename);

    if (file) {
        int c = 0xFF;
        int x = 0;

        while ((c = fgetc(file)) != EOF) {
            if (x >= 64) {
                fprintf(console->out, "\n");
                x = 0;
            }

            x += fprintf(console->out, "%02x", c);
        }

        fclose(file);
        fprintf(console->out, "\n");
    }

    fprintf(console->out, "<<<EOF\n");
}

static command_err_t _console_run_subcommands(command_t* command, int argc, char** argv,
                                              console_t* console) {
    command_err_t err = CMD_SUBCOMMAND_NOT_FOUND;

    if (argc == 0) {
        return CMD_SUBCOMMAND_REQUIRED;
    }

    bool sc_aliased = strlen(argv[0]) == 1;
    size_t sci = 0;
    command_t* sc = NULL;

    while ((sc = command->subcommands[sci++]) != NULL) {

        if ((sc_aliased && argv[0][0] == sc->alias) || !strcasecmp(argv[0], sc->command)) {
            if (sc->func != NULL) {
                err = sc->func(argc - 1, argv + 1, console);
            }
            break;
        }
    }

    return err;
}

void console_run_command(int argc, char** argv, console_t* console) {
    command_err_t err = CMD_NOT_FOUND;

    if (argc == 0)
        return;

    struct cmd_node* ptr = _first;
    bool aliased = strlen(argv[0]) == 1;

    while (ptr != NULL) {
        if ((aliased && argv[0][0] == ptr->command->alias) ||
            !strcasecmp(argv[0], ptr->command->command)) {
            if (ptr->command->subcommands[0] != NULL) {
                err = _console_run_subcommands(ptr->command, argc - 1, argv + 1, console);
            } else if (ptr->command->func != NULL) {
                err = ptr->command->func(argc - 1, argv + 1, console);
            }

            break;
        }

        ptr = ptr->next;
    }

    if (err == CMD_ARG_ERR) {
        fprintf(console->out, "Invalid arguments.\n");
    } else if (err == CMD_NOT_FOUND) {
        fprintf(console->out, "Unknown command: %s\n", argv[0]);
    } else if (err == CMD_SUBCOMMAND_NOT_FOUND) {
        fprintf(console->out, "Unknown subcommand: %s\n", argv[1]);
    } else if (err == CMD_SUBCOMMAND_REQUIRED) {
        fprintf(console->out, "Command requires a subcommand.\n");
    }

    console->err = err;
}