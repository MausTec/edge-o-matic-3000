#include "Console.h"
#include "Hardware.h"
#include "SDHelper.h"
#include "Page.h"
#include "UpdateHelper.h"
#include "config.h"

#include "eom_tscode_handler.h"

#include <SD.h>

#define cmd_f [](char** args, String& out) -> int

typedef int (*cmd_func)(char **, String &);

typedef struct Command {
  char *cmd;
  char *alias;
  char *help;
  cmd_func func;
} Command;


static bool tscode_mode = false;

namespace Console {
  namespace {
    int sh_help(char **, String &);

    int sh_set(char **, String &);

    int sh_list(char **, String &);

    int sh_external(char **, String &);

    int sh_cat(char **, String &);

    int sh_dir(char **, String &);

    int sh_cd(char **, String &);

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
            .cmd = "external",
            .alias = "e",
            .help = "Control the external port",
            .func = &sh_external
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
            .func = cmd_f { ESP.restart(); }
        },
        {
            .cmd = "mode",
            .alias = "m",
            .help = "Set mode automatic|manual",
            .func = cmd_f { RunGraphPage.setMode(args[0]); }
        },
        {
            .cmd = ".setser",
            .alias = nullptr,
            .help = nullptr,
            .func = cmd_f {
              Hardware::setDeviceSerial(args[0]);
            }
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
              out += "Heap (caps_alloc): " + String(xPortGetFreeHeapSize()) + '\n';
              out += "Total heap: " + String(ESP.getHeapSize()) + '\n';
              out += "Free heap: " + String(ESP.getFreeHeap()) + '\n';
              out += "Total PSRAM: " + String(ESP.getPsramSize()) + '\n';
              out += "Free PSRAM: " + String(ESP.getFreePsram()) + '\n';
            },
        },
        {
            .cmd = "heapdump",
            .alias = nullptr,
            .help = nullptr,
            .func = cmd_f {
              heap_caps_dump_all();
            },
        },
        {
            .cmd = ".versionget",
            .alias = nullptr,
            .help = nullptr,
            .func = cmd_f {
              String v = UpdateHelper::checkWebLatestVersion();
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
            .func = cmd_f { out += String(VERSION) + '\n'; },
        },
        {
            .cmd = ".verscmp",
            .alias = nullptr,
            .help = nullptr,
            .func = cmd_f {
              out += String(UpdateHelper::compareVersion(args[0], args[1])) + '\n';
            },
        }
    };

    int sh_help(char **args, String &out) {
      for (int i = 0; i < sizeof(commands) / sizeof(Command); i++) {
        Command c = commands[i];
        if (c.help == nullptr) continue;
        out += (String(c.cmd) + "\t(" + String(c.alias) + ")\t" + String(c.help)) + '\n';
      }
    }

    int sh_external(char **args, String &out) {
#ifdef NG_PLUS
      out += "Not available on this device.\n";
      return 1;
#else
      if (args[0] == NULL) {
        out += "Subcommand required!\n";
        return 1;
      } else if (!strcmp(args[0], "enable")) {
        Hardware::enableExternalBus();
        out += "External bus enabled.\n";
      } else if (!strcmp(args[0], "disable")) {
        Hardware::disableExternalBus();
        out += "External bus disabled.\n";
      } else if (!strcmp(args[0], "slave")) {
        Hardware::enableExternalBus();
        Hardware::joinI2c(I2C_SLAVE_ADDR);
        out += "Joined external bus as slave.\n";
      } else {
        out += "Unknown subcommand!\n";
        return 1;
      }

      return 0;
#endif
    }

    int sh_cat(char **args, String &out) {
      if (args[0] == NULL) {
        out += "Please specify a filename!\n";
        return 1;
      }

      File file = SD.open(cwd + String("/") + String(args[0]));
      if (!file) {
        out += "File not found!\n";
        return 1;
      }

      SDHelper::printFile(file, out);
      return 0;
    }

    int sh_dir(char **args, String &out) {
      File f = SD.open(cwd);
      if (!f) {
        out += "Invalid directory.\n";
        return 1;
      }
      SDHelper::printDirectory(f, 1, out);
      return 0;
    }

    int sh_cd(char **args, String &out) {
      if (args[0] == NULL) {
        out += "Directory required.";
        return 1;
      }

      String newDir;
      if (!strcmp(args[0], "..")) {
        newDir = cwd.substring(0, cwd.lastIndexOf("/"));
      } else {
        newDir = cwd + String("/") + String(args[0]);
      }

      File f = SD.open(newDir);
      if (!f || !f.isDirectory()) {
        out += "Invalid directory.\n";
        return 1;
      }
      cwd = newDir;
      f.close();
    }

    int sh_set(char **args, String &out) {
      char *option = args[0];
      bool require_reboot = false;

      if (option == NULL) {
        out += "An option is required.\n";
        return 1;
      }

      if (args[1] == NULL) {
        if (!getConfigValue(args[0], out)) {
          out += "Unknown config key!\n";
        }
      } else {
        if (setConfigValue(args[0], args[1], require_reboot)) {
          saveConfigToSd(0);

          if (require_reboot) {
            out += ("A device reset will be required for the new settings to "
                    "take effect.\n");
          }
        } else {
          out += "Unknown config key!\n";
        }
      }
    }

    int sh_list(char **args, String &out) {
      dumpConfigToJson(out);
    }
  }

  /**
   * Handles the message currently in the buffer.
   */
  void handleMessage(char *line, String &out) {
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
    String response;
    Serial.println("> " + String(line));
    handleMessage(line, response);
    Serial.println(response);
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
    while (Serial.available() > 0) {
      // read the incoming byte:
      char incoming = Serial.read();

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
          Serial.println(response);
        }

        buffer_i = 0;
        buffer[buffer_i] = 0;
      } else {
        buffer[buffer_i] = incoming;
        buffer_i++;
        buffer[buffer_i] = 0;
      }
    }
  }
}