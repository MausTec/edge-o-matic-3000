#include "Console.h"
#include "Hardware.h"
#include "SDHelper.h"
#include "Page.h"
#include "UpdateHelper.h"
#include "config.h"
#include "OrgasmControl.h"
#include "config_defs.h"
#include <string>
#include <sys/stat.h>

#include "eom_tscode_handler.h"

// #include <SD.h>

#define cmd_f [](char** args, std::string& out) -> int

typedef int (*cmd_func)(char **, std::string &);

typedef struct Command {
  char *cmd;
  char *alias;
  char *help;
  cmd_func func;
} Command;


static bool tscode_mode = false;

namespace Console {
  namespace {
    int sh_help(char **, std::string &);

    int sh_set(char **, std::string &);

    int sh_list(char **, std::string &);

    int sh_cat(char **, std::string &);

    int sh_dir(char **, std::string &);

    int sh_cd(char **, std::string &);

    Command commands[] = {
        {
            .cmd = "help",
            .alias = "?",
            .help = "Get help, if you need it.",
            .func = &sh_help
        },
        {
            .cmd = "set",
            .alias = "s",
            .help = "Set a config value",
            .func = &sh_set
        },
        {
            .cmd = "list",
            .alias = "l",
            .help = "List all config in JSON format",
            .func = &sh_list
        },
        {
            .cmd = "cat",
            .alias = "c",
            .help = "Print a file to the serial console",
            .func = &sh_cat
        },
        {
            .cmd = "dir",
            .alias = "d",
            .help = "List current directory",
            .func = &sh_dir
        },
        {
            .cmd = "cd",
            .alias = ".",
            .help = "Change directory",
            .func = &sh_cd
        },
        {
            .cmd = "restart",
            .alias = "R",
            .help = "Restart the device",
            .func = cmd_f { out += "temporarily broken"; }
        },
        {
            .cmd = "mode",
            .alias = "m",
            .help = "Set mode automatic|manual",
            .func = cmd_f { RunGraphPage.setMode(args[0]); }
        },
        {
            .cmd = ".getser",
            .alias = nullptr,
            .help = nullptr,
            .func = cmd_f {
              out += "Device Serial: ";
              out += Hardware::getDeviceSerial();
            }
        },
        {
            .cmd = ".debugsens",
            .alias = nullptr,
            .help = nullptr,
            .func = cmd_f {
              out += "Set: ";
              out += Config.sensor_sensitivity;
              out += ", Actual: ";
              out += Hardware::getPressureSensitivity();
            },
        },
        {
            .cmd = ".getver",
            .alias = nullptr,
            .help = nullptr,
            .func = cmd_f {
              out += VERSION;
            }
        },
        {
            .cmd = "free",
            .alias = "f",
            .help = "Get free heap space",
            .func = cmd_f {
              // out += "Heap (caps_alloc): " + std::to_string(xPortGetFreeHeapSize()) + '\n';
              // out += "Total heap: " + std::to_string(ESP.getHeapSize()) + '\n';
              // out += "Free heap: " + std::to_string(ESP.getFreeHeap()) + '\n';
              // out += "Total PSRAM: " + std::to_string(ESP.getPsramSize()) + '\n';
              // out += "Free PSRAM: " + std::to_string(ESP.getFreePsram()) + '\n';
              out += "temporarily not supported";
            },
        },
        {
            .cmd = ".versionget",
            .alias = nullptr,
            .help = nullptr,
            .func = cmd_f {
              std::string v = UpdateHelper::checkWebLatestVersion();
              out += v + '\n';

              if (UpdateHelper::compareVersion(v.c_str(), VERSION) > 0) {
                out += "An update is available!\n";
              }
            },
        },
        {
            .cmd = "version",
            .alias = "v",
            .help = "Get current firmware version",
            .func = cmd_f { out += VERSION "\n"; },
        },
        {
            .cmd = ".verscmp",
            .alias = nullptr,
            .help = nullptr,
            .func = cmd_f {
              out += std::to_string(UpdateHelper::compareVersion(args[0], args[1])) + '\n';
            },
        },
        {
            .cmd = "orgasm",
            .alias = "o",
            .help = "permit Orgasm : Set Post Orgasm Seconds ( orgasm 10 )",
            .func = cmd_f { 
              int seconds = atoi(args[0]);
              OrgasmControl::permitOrgasmNow(seconds); },
        },
        {
            .cmd = "lock",
            .alias = "lock",
            .help = "lockMenu true/false",
            .func = cmd_f { 
              bool value = atob(args[0]);
              OrgasmControl::lockMenuNow(value); },
        }
    };

    int sh_help(char **args, std::string &out) {
      for (int i = 0; i < sizeof(commands) / sizeof(Command); i++) {
        Command c = commands[i];
        if (c.help == nullptr) continue;
        out = out + c.cmd + "\t(" + c.alias + ")\t" + c.help + "\n";
      }
    }

    int sh_cat(char **args, std::string &out) {
      if (args[0] == NULL) {
        out += "Please specify a filename!\n";
        return 1;
      }

      struct stat st;
      char path[MAX_DIR_LENGTH] = "";
      if (stat(path, &st) != 0 || S_ISDIR(st.st_mode)) {
        out += "Invalid file.\n";
        return 1;
      }

      // SDHelper::printFile(file, out);
      // return 0;
    }

    int sh_dir(char **args, std::string &out) {
      struct stat st;
      if (stat(cwd, &st) != 0 || !S_ISDIR(st.st_mode)) {
        out += "Invalid directory.\n";
        return 1;
      }

      // SDHelper::printDirectory(f, 1, out);
      return 0;
    }

    int sh_cd(char **args, std::string &out) {
      if (args[0] == NULL) {
        out += "Directory required.";
        return 1;
      }

      char newDir[MAX_DIR_LENGTH] = "";
      if (!strcmp(args[0], "..")) {
        char *last = strrchr(cwd, '/');
        if (last != NULL) {
          strncpy(newDir, cwd, strrchr(cwd, '/') - cwd);
        }
      } else {
        strlcpy(newDir, cwd, MAX_DIR_LENGTH);
        strlcat(newDir, "/", MAX_DIR_LENGTH);
        strlcat(newDir, args[0], MAX_DIR_LENGTH);
      }

      struct stat st;
      if (stat(newDir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        out += "Invalid directory.\n";
        return 1;
      }

      strlcpy(cwd, newDir, MAX_DIR_LENGTH);
    }

    int sh_set(char **args, std::string &out) {
      char *option = args[0];
      bool require_reboot = false;

      if (option == NULL) {
        out += "An option is required.\n";
        return 1;
      }

      if (args[1] == NULL) {
        char buffer[120];
        if (!get_config_value(args[0], buffer, 120)) {
          out += "Unknown config key!\n";
        }
      } else {
        if (set_config_value(args[0], args[1], &require_reboot)) {
          save_config_to_sd(0);

          if (require_reboot) {
            out += ("A device reset will be required for the new settings to "
                    "take effect.\n");
          }
        } else {
          out += "Unknown config key!\n";
        }
      }
    }

    int sh_list(char **args, std::string &out) {
      // dumpConfigToJson(out);
    }
  }

  /**
   * Handles the message currently in the buffer.
   */
  void handleMessage(char *line, std::string &out) {
    const int TOK_BUFSIZE = 64;
    const char *TOK_DELIM = " \t\r\n\a";

    int bufsize = TOK_BUFSIZE;
    int position = 0;

    char *tokens[bufsize] = {0};
    char *token;

    if (!tokens) {
      out += "allocation error :(";
      return;
    }

    token = strtok(line, TOK_DELIM);
    while (token != NULL) {
      tokens[position] = token;
      position++;

      if (position > bufsize) {
        // buffer overflow, extend?
      }

      token = strtok(NULL, TOK_DELIM);
    }

    tokens[position] = 0;

    // Empty command!
    if (position == 0) {
      return;
    }

    // Find Cmd
    char *cmd = tokens[0];
    bool handled = false;

    for (int i = 0; cmd[i]; i++) {
      // Command to lower
      cmd[i] = tolower(cmd[i]);
    }

    for (int i = 0; i < sizeof(commands) / sizeof(Command); i++) {
      Command c = commands[i];

      if (strcmp(c.cmd, cmd) == 0 || (c.alias != nullptr && strcmp(c.alias, cmd) == 0)) {
        c.func(tokens + 1, out);
        handled = true;
        break;
      }
    }

    if (!handled) {
      out += "Unknown command.";
    }
  }

  void handleMessage(char *line) {
    std::string response;
    printf("> %s\n", line);
    handleMessage(line, response);
    puts(response.c_str());
  }

  /**
   * Intercept TS-code commands to see if we're changing modes, otherwise pass to global handler.
   */
  tscode_command_response_t tscode_handler(tscode_command_t* cmd, char* response, size_t resp_len) {
      if (cmd->type == TSCODE_EXIT_TSCODE_MODE) {
        tscode_mode = 0;
        return TSCODE_RESPONSE_OK;
      } else {
        return eom_tscode_handler(cmd, response, resp_len);
      }
  }

  /**
   * Reads and stores incoming bytes onto the buffer until
   * we have a newline.
   */
  void loop() {
    char incoming = fgetc(stdin);
    while (incoming != 0xFF) {
      if (incoming == '\r') {
        continue;
      } else if (incoming == '\n') {
        if (!tscode_mode) {
          if (!strcmp(buffer, "TSCODE")) {
            tscode_mode = true;
          } else {
            handleMessage(buffer);
          }
        } else {
          char response[121] = "";
          tscode_process_buffer(buffer, tscode_handler, response, 120);
          puts(response);
        }

        buffer_i = 0;
        buffer[buffer_i] = 0;
      } else {
        buffer[buffer_i] = incoming;
        buffer_i++;
        buffer[buffer_i] = 0;
      }

      incoming = fgetc(stdin);
    }
  }
}