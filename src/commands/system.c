#include "commands/index.h"
#include "console.h"

static command_err_t cmd_system_restart(int argc, char** argv, console_t* console) {
    if (argc != 0) { return CMD_ARG_ERR; }
    fprintf(console->out, "Device going down for restart, like, NOW!\n");

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
    if (argc == 0) { return CMD_ARG_ERR; }
    return CMD_OK;
}

static const command_t cmd_system_time_s = {
    .command = "time",
    .help = "Get system time.",
    .alias = NULL,
    .func = &cmd_system_time,
    .subcommands = {NULL},
};

static const command_t cmd_system_s = {
    .command = "system",
    .help = "System control",
    .alias = NULL,
    .func = NULL,
    .subcommands = {
        &cmd_system_restart_s,
        NULL
    }
};

void commands_register_system(void) {
    console_register_command(&cmd_system_s);
}