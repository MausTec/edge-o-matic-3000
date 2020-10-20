#include "../include/Console.h"
#include "../include/Hardware.h"
#include "../include/SDHelper.h"
#include "../config.h"

#include <SD.h>

typedef int (*cmd_func)(char**, String&);

typedef struct Command {
  char *cmd;
  char *alias;
  char *help;
  cmd_func func;
} Command;

/**
 * Cast a string to a bool. Accepts "false", "no", "off", "0" to false, all other
 * strings cast to true.
 * @param a
 * @return
 */
bool atob(char *a) {
  for (int i = 0; a[i]; i++) {
    a[i] = tolower(a[i]);
  }

  return !(
      strcmp(a, "false") == 0 ||
      strcmp(a, "no") == 0 ||
      strcmp(a, "off") == 0 ||
      strcmp(a, "0") == 0
  );
}

namespace Console {
  namespace {
    int sh_help(char**, String&);
    int sh_set(char**, String&);
    int sh_list(char**, String&);
    int sh_external(char**, String&);
    int sh_bool(char**, String&);
    int sh_cat(char**, String&);
    int sh_dir(char**, String&);
    int sh_cd(char**, String&);

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
        .cmd = "bool",
        .alias = "b",
        .help = "Test the bool cast",
        .func = &sh_bool
      }
    };

    int sh_help(char **args, String &out) {
      for (int i = 0; i < sizeof(commands)/sizeof(Command); i++) {
        Command c = commands[i];
        out += (String(c.cmd) + "\t(" + String(c.alias) + ")\t" + String(c.help)) + '\n';
      }
    }

    int sh_external(char **args, String &out) {
      if (args[0] == NULL) {
        out += "Subcommand required!\n";
        return 1;
      } else if (! strcmp(args[0], "enable")) {
        Hardware::enableExternalBus();
        out += "External bus enabled.\n";
      } else if (! strcmp(args[0], "disable")) {
        Hardware::disableExternalBus();
        out += "External bus disabled.\n";
      } else if (! strcmp(args[0], "slave")) {
        Hardware::enableExternalBus();
        Hardware::joinI2c(I2C_SLAVE_ADDR);
        out += "Joined external bus as slave.\n";
      } else {
        out += "Unknown subcommand!\n";
        return 1;
      }

      return 0;
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
      if (! strcmp(args[0], "..")) {
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

      // Go through boolean + int values first:
      if (!strcmp(option, "wifi_on")) {
        if (!args[1]) {
          out += String(Config.wifi_on) + '\n';
          return 0;
        }
        Config.wifi_on = atob(args[1]);
        require_reboot = true;
      } else if(!strcmp(option, "bt_on")) {
        if (!args[1]) {
          out += String(Config.bt_on) + '\n';
          return 0;
        }
        Config.bt_on = atob(args[1]);
        require_reboot = true;
      } else if(!strcmp(option, "led_brightness")) {
        if (!args[1]) {
          out += String(Config.led_brightness) + '\n';
          return 0;
        }
        Config.led_brightness = atoi(args[1]);
      } else if(!strcmp(option, "websocket_port")) {
        if (!args[1]) {
          out += String(Config.websocket_port) + '\n';
          return 0;
        }
        Config.websocket_port = atoi(args[1]);
        require_reboot = true;
      } else if(!strcmp(option, "motor_max_speed")) {
        if (!args[1]) {
          out += String(Config.motor_max_speed) + '\n';
          return 0;
        }
        Config.motor_max_speed = atoi(args[1]);
      } else if(!strcmp(option, "screen_dim_seconds")) {
        if (!args[1]) {
          out += String(Config.screen_dim_seconds) + '\n';
          return 0;
        }
        Config.screen_dim_seconds = atoi(args[1]);
      } else if(!strcmp(option, "pressure_smoothing")) {
        if (!args[1]) {
          out += String(Config.pressure_smoothing) + '\n';
          return 0;
        }
        Config.pressure_smoothing = atoi(args[1]);
      } else if(!strcmp(option, "classic_serial")) {
        if (!args[1]) {
          out += String(Config.classic_serial) + '\n';
          return 0;
        }
        Config.classic_serial = atob(args[1]);
      } else if(!strcmp(option, "use_average_values")) {
        if (!args[1]) {
          out += String(Config.use_average_values) + '\n';
          return 0;
        }
        Config.use_average_values = atob(args[1]);
      } else if(!strcmp(option, "sensitivity_threshold")) {
        if (!args[1]) {
          out += String(Config.sensitivity_threshold) + '\n';
          return 0;
        }
        Config.sensitivity_threshold = atoi(args[1]);
      } else if(!strcmp(option, "motor_ramp_time_s")) {
        if (!args[1]) {
          out += String(Config.motor_ramp_time_s) + '\n';
          return 0;
        }
        Config.motor_ramp_time_s = atoi(args[1]);
      } else if(!strcmp(option, "update_frequency_hz")) {
        if (!args[1]) {
          out += String(Config.update_frequency_hz) + '\n';
          return 0;
        }
        Config.update_frequency_hz = atoi(args[1]);
      } else if(!strcmp(option, "sensor_sensitivity")) {
        if (!args[1]) {
          out += String(Config.sensor_sensitivity) + '\n';
          return 0;
        }
        Config.sensor_sensitivity = atoi(args[1]);
        Hardware::setPressureSensitivity(Config.sensor_sensitivity);
      } else if (!strcmp(option, "knob_rgb")) {
        if (!args[1] || !args[2] || !args[3]) {
          out += ("Usage: set knob_rgb r g b") + '\n';
          return 1;
        }
        Hardware::setEncoderColor(CRGB(atoi(args[1]), atoi(args[2]), atoi(args[3])));
      } else if (!strcmp(option, "wifi_ssid")) {
        if (!args[1]) {
          out += String(Config.wifi_ssid) + '\n';
          return 0;
        }
        strlcpy(Config.wifi_ssid, args[1], sizeof(Config.wifi_ssid));
      } else if (!strcmp(option, "wifi_key")) {
        if (!args[1]) {
          out += String(Config.wifi_key) + '\n';
          return 0;
        }
        strlcpy(Config.wifi_key, args[1], sizeof(Config.wifi_key));
      } else if (!strcmp(option, "bt_display_name")) {
        if (!args[1]) {
          out += String(Config.bt_display_name) + '\n';
          return 0;
        }
        strlcpy(Config.bt_display_name, args[1], sizeof(Config.bt_display_name));
      } else {
        out += ("Unknown config key: " + String(option)) + '\n';
        return 1;
      }

      saveConfigToSd(0);

      if (require_reboot) {
        out += ("A device reset will be required for the new settings to "
                "take effect.\n");
      }
    }

    int sh_list(char **args, String &out) {
      dumpConfigToJson(out);
    }

    int sh_bool(char **args, String &out) {
      int p = 0;
      char *c = args[p];
      while (c != NULL) {
        out += (String(c) + "\t");
        out += String(atob(c) ? "TRUE" : "FALSE") + '\n';
        p++;
        c = args[p];
      }
    }
  }

  /**
   * Handles the message currently in the buffer.
   */
  void handleMessage(char *line, String &out) {
    const int TOK_BUFSIZE = 64;
    const char* TOK_DELIM = " \t\r\n\a";

    int bufsize = TOK_BUFSIZE;
    int position = 0;

    char *tokens[bufsize] = { 0 };
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

    for (int i = 0; i < sizeof(commands)/sizeof(Command); i++) {
      Command c = commands[i];

      if (strcmp(c.cmd, cmd) == 0 || strcmp(c.alias, cmd) == 0) {
        c.func(tokens+1, out);
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
   * Reads and stores incoming bytes onto the buffer until
   * we have a newline.
   */
  void loop() {
    while (Serial.available() > 0) {
      // read the incoming byte:
      char incoming = Serial.read();

      if (incoming == '\n') {
        handleMessage(buffer);
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