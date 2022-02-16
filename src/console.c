#include "console.h"

#include "SDHelper.h"

#include "config.h"
#include "config_defs.h"

#include <string.h>
#include <sys/stat.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "commands/index.h"
#include "linenoise/linenoise.h"
#include "driver/uart.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "eom-hal.h"

#define PROMPT "eom:%s> "
#define ARGV_MAX 16
#define CMDLINE_MAX 256

static const char* TAG = "console";
static esp_console_repl_t* _repl = NULL;

char _history_file[SD_MAX_DIR_LENGTH + 1] = { 0 };

static struct cmd_node {
    command_t command;
    struct cmd_node* next;
} *_first = NULL;

static command_err_t cmd_help(int argc, char **argv, console_t *console) {
    struct cmd_node *ptr = _first;
    fprintf(console->out, "Available commands:\n\n");
    while (ptr != NULL) {
        fprintf(console->out, "    %-10s  %s\n", ptr->command.command, ptr->command.help);
        ptr = ptr->next;
    }
    return CMD_OK;
}

static void register_help(void) {
    const command_t cmd = {
        .command = "help",
        .help = "Get help about all or a specific command.",
        .alias = '?',
        .func = &cmd_help,
    };

    console_register_command(&cmd);
}

static void console_task(void *args) {
    char prompt[80] = { 0 };

    console_t uart_console = {
        .out = stdout,
    };

    strncpy(uart_console.cwd, eom_hal_get_sd_mount_point(), PATH_MAX);

    printf("Press any key to initialize the terminal.\n");
    for (;;) {
        if (getc(stdin) != EOF) break;
        vTaskDelay(1);
    }

    if (Config.store_command_history) {
        SDHelper_getAbsolutePath(_history_file, SD_MAX_DIR_LENGTH, "history.txt");
        linenoiseHistoryLoad(_history_file);
    }

    if (linenoiseProbe()) {
        printf("Your terminal does not support escape sequences. This experience may\n"
               "not be as pretty as on other consoles.\n");
        linenoiseSetDumbMode(true);
    }

    for (;;) {
        snprintf(prompt, 79, PROMPT, uart_console.cwd);
        char *line = linenoise(prompt);

        if (line[0] != '\0') {
            linenoiseHistoryAdd(line);
            
            if (_history_file[0] != '\0') {
                linenoiseHistorySave(_history_file);
            }

            char *argv[ARGV_MAX] = { 0 };
            int argc = esp_console_split_argv(line, argv, ARGV_MAX);
            console_run_command(argc, argv, &uart_console);
        }

        linenoiseFree(line);
    }

    vTaskDelete(NULL);
}

void console_init(void) {
    register_help();
    commands_register_all();

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

    linenoiseAllowEmpty(true);
    linenoiseSetMultiLine(true);
    linenoiseHistorySetMaxLen(100);
    linenoiseAllowEmpty(true);
}

void console_register_command(command_t* command) {
    struct cmd_node *n = (struct cmd_node*) malloc(sizeof(struct cmd_node));
    memcpy(&n->command, command, sizeof(command_t));
    n->next = NULL;

    if (_first == NULL) {
        _first = n;
    } else {
        struct cmd_node *ptr = _first;
        while (ptr->next != NULL) ptr = ptr->next;
        ptr->next = n;
    }
}

void console_ready(void) {
    xTaskCreate(
        &console_task,
        "CONSOLE",
        8192,
        NULL,
        tskIDLE_PRIORITY,
        NULL
    );
}

void console_send_file(const char* filename, console_t* console) {
    FILE* file = fopen(filename, "rb");
    long size = -1;

    if (file) {
        fseek(file, 0, SEEK_END);
        size = ftell(file);
        rewind(file);
    }

    fprintf(console->out, ">>>HEXFILE:%d;%s\n", size, filename);

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

void console_run_command(int argc, char **argv, console_t *console) {
    if (argc == 0)
        return;

    char *cmdstr = argv[0];
    for (int i = 0; i < strlen(cmdstr); i++) {
        cmdstr[i] = tolower(cmdstr[i]);
    }

    struct cmd_node *ptr = _first;
    bool aliased = strlen(argv[0]) == 1;

    while (ptr != NULL) {
        if ((aliased && cmdstr[0] == ptr->command.alias) || !strcmp(cmdstr, ptr->command.command)) {
            command_err_t err = ptr->command.func(argc-1, argv+1, console);
            
            if (err == CMD_ARG_ERR) {
                fprintf(console->out, "Invalid arguments.\n");
            }

            return;
        }

        ptr = ptr->next;
    }

    fprintf(console->out, "Unknown command: %s\n", cmdstr);
}

// #include "Hardware.h"
// #include "SDHelper.h"
// #include "Page.h"
// #include "UpdateHelper.h"
// #include "OrgasmControl.h"

// #include "eom_tscode_handler.h"

// // #include <SD.h>

// #define cmd_f [](char** argv, size_t argc, std::string &out) -> int

// typedef int (*cmd_func)(char **argv, size_t argc, std::string &out);


// typedef struct Command {
//   char *cmd;
//   char *alias;
//   char *help;
//   cmd_func func;
// } Command;

// static bool tscode_mode = false;

// namespace Console {
//   namespace {
//     int sh_help(char **argv, size_t argc, std::string &out);
//     int sh_set(char **argv, size_t argc, std::string &out);
//     int sh_list(char **argv, size_t argc, std::string &out);
//     int sh_cat(char **argv, size_t argc, std::string &out);
//     int sh_dir(char **argv, size_t argc, std::string &out);
//     int sh_cd(char **argv, size_t argc, std::string &out);

//     Command commands[] = {
//         {
//             .cmd = "help",
//             .alias = "?",
//             .help = "Get help, if you need it.",
//             .func = &sh_help
//         },
//         {
//             .cmd = "set",
//             .alias = "s",
//             .help = "Set a config value",
//             .func = &sh_set
//         },
//         {
//             .cmd = "list",
//             .alias = "l",
//             .help = "List all config in JSON format",
//             .func = &sh_list
//         },
//         {
//             .cmd = "cat",
//             .alias = "c",
//             .help = "Print a file to the serial console",
//             .func = &sh_cat
//         },
//         {
//             .cmd = "dir",
//             .alias = "d",
//             .help = "List current directory",
//             .func = &sh_dir
//         },
//         {
//             .cmd = "cd",
//             .alias = ".",
//             .help = "Change directory",
//             .func = &sh_cd
//         },
//         {
//             .cmd = "restart",
//             .alias = "R",
//             .help = "Restart the device",
//             .func = cmd_f { out += "temporarily broken"; }
//         },
//         {
//             .cmd = "mode",
//             .alias = "m",
//             .help = "Set mode automatic|manual",
//             .func = cmd_f { RunGraphPage.setMode(argv[0]); }
//         },
//         {
//             .cmd = ".getser",
//             .alias = nullptr,
//             .help = nullptr,
//             .func = cmd_f {
//               out += "Device Serial: ";
//               out += Hardware::getDeviceSerial();
//             }
//         },
//         {
//             .cmd = ".debugsens",
//             .alias = nullptr,
//             .help = nullptr,
//             .func = cmd_f {
//               out += "Set: ";
//               out += Config.sensor_sensitivity;
//               out += ", Actual: ";
//               out += Hardware::getPressureSensitivity();
//             },
//         },
//         {
//             .cmd = ".getver",
//             .alias = nullptr,
//             .help = nullptr,
//             .func = cmd_f {
//               out += VERSION;
//             }
//         },
//         {
//             .cmd = "free",
//             .alias = "f",
//             .help = "Get free heap space",
//             .func = cmd_f {
//               // out += "Heap (caps_alloc): " + std::to_string(xPortGetFreeHeapSize()) + '\n';
//               // out += "Total heap: " + std::to_string(ESP.getHeapSize()) + '\n';
//               // out += "Free heap: " + std::to_string(ESP.getFreeHeap()) + '\n';
//               // out += "Total PSRAM: " + std::to_string(ESP.getPsramSize()) + '\n';
//               // out += "Free PSRAM: " + std::to_string(ESP.getFreePsram()) + '\n';
//               out += "temporarily not supported";
//             },
//         },
//         {
//             .cmd = ".versionget",
//             .alias = nullptr,
//             .help = nullptr,
//             .func = cmd_f {
//               std::string v = UpdateHelper::checkWebLatestVersion();
//               out += v + '\n';

//               if (UpdateHelper::compareVersion(v.c_str(), VERSION) > 0) {
//                 out += "An update is available!\n";
//               }
//             },
//         },
//         {
//             .cmd = "version",
//             .alias = "v",
//             .help = "Get current firmware version",
//             .func = cmd_f { out += VERSION "\n"; },
//         },
//         {
//             .cmd = ".verscmp",
//             .alias = nullptr,
//             .help = nullptr,
//             .func = cmd_f {
//               out += std::to_string(UpdateHelper::compareVersion(argv[0], argv[1])) + '\n';
//             },
//         },
//         {
//             .cmd = "orgasm",
//             .alias = "o",
//             .help = "permit Orgasm : Set Post Orgasm Seconds ( orgasm 10 )",
//             .func = cmd_f { 
//               int seconds = atoi(argv[0]);
//               OrgasmControl::permitOrgasmNow(seconds); },
//         },
//         {
//             .cmd = "lock",
//             .alias = "lock",
//             .help = "lockMenu true/false",
//             .func = cmd_f { 
//               bool value = atob(argv[0]);
//               OrgasmControl::lockMenuNow(value); },
//         }
//     };

//     int sh_help(char **args, size_t argc, std::string &out) {
//       for (int i = 0; i < sizeof(commands) / sizeof(Command); i++) {
//         Command c = commands[i];
//         if (c.help == nullptr) continue;
//         out = out + c.cmd + "\t(" + c.alias + ")\t" + c.help + "\n";
//       }

//       return 0;
//     }

//     int sh_cat(char **args, size_t argc, std::string &out) {
//       if (args[0] == NULL) {
//         out += "Please specify a filename!\n";
//         return 1;
//       }

//       struct stat st;
//       char path[MAX_DIR_LENGTH] = "";
//       strlcpy(path, cwd, MAX_DIR_LENGTH);
//       SDHelper_join(path, MAX_DIR_LENGTH, args[0]);

//       if (stat(path, &st) != 0 || S_ISDIR(st.st_mode)) {
//         out += "Invalid file: ";
//         out += path;
//         return 1;
//       }

//       puts("");
//       SDHelper_logFile(path);
//       return 0;
//     }

//     int sh_dir(char **args, size_t argc, std::string &out) {
//       // struct stat st;
//       // if (stat(cwd, &st) != 0 || !S_ISDIR(st.st_mode)) {
//       //   out += "Invalid directory.\n";
//       //   return 1;
//       // }

//       puts(""); // hacking for now.
//       if (argc > 0) {
//         SDHelper_logDirectory(args[0]);
//       } else {
//         SDHelper_logDirectory(cwd);
//       }

//       return 0;
//     }

//     int sh_cd(char **args, size_t argc, std::string &out) {
//       if (args[0] == NULL) {
//         out += "Directory required.";
//         return 1;
//       }

//       char newDir[MAX_DIR_LENGTH] = "";
//       if (!strcmp(args[0], "..")) {
//         char *last = strrchr(cwd, '/');
//         if (last != NULL) {
//           strncpy(newDir, cwd, strrchr(cwd, '/') - cwd);
//         }
//       } else {
//         strlcpy(newDir, cwd, MAX_DIR_LENGTH);
//         SDHelper_join(newDir, MAX_DIR_LENGTH, args[0]);
//       }

//       struct stat st;
//       if (stat(newDir, &st) != 0 || !S_ISDIR(st.st_mode)) {
//         out += "Invalid directory: ";
//         out += newDir;
//         out += "\n";
//         return 1;
//       }

//       strlcpy(cwd, newDir, MAX_DIR_LENGTH);
//       out += cwd;
//       return 0;
//     }

//     int sh_set(char **args, size_t argc, std::string &out) {
//       char *option = args[0];
//       bool require_reboot = false;

//       if (option == NULL) {
//         out += "An option is required.\n";
//         return 1;
//       }

//       if (args[1] == NULL) {
//         char buffer[120];
//         if (!get_config_value(args[0], buffer, 120)) {
//           out += "Unknown config key!\n";
//         }
//       } else {
//         if (set_config_value(args[0], args[1], &require_reboot)) {
//           save_config_to_sd(0);

//           if (require_reboot) {
//             out += ("A device reset will be required for the new settings to "
//                     "take effect.\n");
//           }
//         } else {
//           out += "Unknown config key!\n";
//         }
//       }

//       return 0;
//     }

//     int sh_list(char **args, size_t argc, std::string &out) {
//       // dumpConfigToJson(out);
//     }
//   }

//   /**
//    * Handles the message currently in the buffer.
//    */
//   void handleMessage(char *line, std::string &out) {
//     const int TOK_BUFSIZE = 64;
//     const char *TOK_DELIM = " \t\r\n\a";

//     int bufsize = TOK_BUFSIZE;
//     int position = 0;

//     char *tokens[bufsize] = {0};
//     char *token;

//     if (!tokens) {
//       out += "allocation error :(";
//       return;
//     }

//     token = strtok(line, TOK_DELIM);
//     while (token != NULL) {
//       tokens[position] = token;
//       position++;

//       if (position > bufsize) {
//         // buffer overflow, extend?
//       }

//       token = strtok(NULL, TOK_DELIM);
//     }

//     tokens[position] = 0;

//     // Empty command!
//     if (position == 0) {
//       return;
//     }

//     // Find Cmd
//     char *cmd = tokens[0];
//     bool handled = false;

//     for (int i = 0; cmd[i]; i++) {
//       // Command to lower
//       cmd[i] = tolower(cmd[i]);
//     }

//     for (int i = 0; i < sizeof(commands) / sizeof(Command); i++) {
//       Command c = commands[i];

//       if (strcmp(c.cmd, cmd) == 0 || (c.alias != nullptr && strcmp(c.alias, cmd) == 0)) {
//         c.func(tokens + 1, position - 1, out);
//         handled = true;
//         break;
//       }
//     }

//     if (!handled) {
//       out += "Unknown command.";
//     }
//   }

//   /**
//    * Intercept TS-code commands to see if we're changing modes, otherwise pass to global handler.
//    */
//   tscode_command_response_t tscode_handler(tscode_command_t* cmd, char* response, size_t resp_len) {
//       if (cmd->type == TSCODE_EXIT_TSCODE_MODE) {
//         tscode_mode = 0;
//         return TSCODE_RESPONSE_OK;
//       } else {
//         return eom_tscode_handler(cmd, response, resp_len);
//       }
//   }

//   void ready() {
//     printf("\n" PROMPT, cwd);
//   }

//   /**
//    * Reads and stores incoming bytes onto the buffer until
//    * we have a newline.
//    */
//   void loop() {
//     char incoming = fgetc(stdin);
//     while (incoming != 0xFF) {
//       if (incoming == '\r') {
//       } else if (incoming == '\n') {
//         if (!tscode_mode) {
//           if (!strcmp(buffer, "TSCODE")) {
//             tscode_mode = true;
//           } else {
//             std::string response;
//             handleMessage(buffer, response);
//             const char *str = response.c_str();
//             puts("");
//             if (str[0] != '\0') {
//               puts(str);
//               if (str[strlen(str)-1] != '\n') printf("\n");
//             }
//             printf(PROMPT, cwd);
//           }
//         } else {
//           char response[121] = "";
//           tscode_process_buffer(buffer, tscode_handler, response, 120);
//           puts(response);
//         }

//         buffer_i = 0;
//         buffer[buffer_i] = 0;
//       } else if (incoming == '\b') {
//         buffer_i--;
//         buffer[buffer_i] = 0;
//         printf("\b \b");
//       } else if (!isprint(incoming)) {
//         printf("?0x%02x\b\b\b\b\b", incoming);
//       } else {
//         fputc(incoming, stdout);
//         buffer[buffer_i] = incoming;
//         buffer_i++;
//         buffer[buffer_i] = 0;
//       }

//       incoming = fgetc(stdin);
//     }
//   }
// }