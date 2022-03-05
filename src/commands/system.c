#include "commands/index.h"
#include "console.h"
#include "eom-hal.h"
#include "esp_system.h"

static command_err_t cmd_system_restart(int argc, char** argv, console_t* console) {
    if (argc != 0) { return CMD_ARG_ERR; }
    fprintf(console->out, "Device going down for restart, like, NOW!\n");
    fflush(stdout);
    esp_restart();

    return CMD_OK;
}

static const command_t cmd_system_restart_s = {
    .command = "restart",
    .help = "Restart your device",
    .alias = NULL,
    .func = &cmd_system_restart,
    .subcommands = {NULL},
};

static command_err_t cmd_system_time(int argc, char** argv, console_t* console) {
    if (argc != 0) { return CMD_ARG_ERR; }
    time_t now;
    time(&now);
    fprintf(console->out, "Time: %s", ctime(&now));
    return CMD_OK;
}

static const command_t cmd_system_time_s = {
    .command = "time",
    .help = "Get system time",
    .alias = NULL,
    .func = &cmd_system_time,
    .subcommands = {NULL},
};

static command_err_t cmd_system_color(int argc, char** argv, console_t* console) {
    if (argc == 3) {
        eom_hal_color_t color = {
            .r = atoi(argv[0]),
            .g = atoi(argv[1]),
            .b = atoi(argv[2]),
        };

        eom_hal_set_encoder_color(color);
    } else if (argc == 1) {
        char *ptr = argv[0];
        if (ptr[0] == '#') ptr++;

        uint32_t color_hex = strtol(ptr, NULL, 16);
        eom_hal_color_t color = {
            .r = (color_hex >> 16) & 0xFF,
            .g = (color_hex >> 8) & 0xFF,
            .b = (color_hex >> 0) & 0xFF,
        };

        eom_hal_set_encoder_color(color);
    } else {
        return CMD_ARG_ERR;
    }

    return CMD_OK;
}

static const command_t cmd_system_color_s = {
    .command = "color",
    .help = "Set the illumination color of the device",
    .alias = NULL,
    .func = &cmd_system_color,
    .subcommands = {NULL},
};

static const command_t cmd_system_s = {
    .command = "system",
    .help = "System control",
    .alias = NULL,
    .func = NULL,
    .subcommands = {
        &cmd_system_restart_s,
        &cmd_system_time_s,
        &cmd_system_color_s,
        NULL
    }
};

void commands_register_system(void) {
    console_register_command(&cmd_system_s);
}