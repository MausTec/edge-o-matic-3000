#include "console.h"
#include "commands/index.h"
#include "config.h"
#include "config_defs.h"
#include "SDHelper.h"

static command_err_t cmd_config_list(int argc, char** argv, console_t* console) {
    fprintf(console->out, "Config defined in: %s\n\n", Config._filename);
    
    cJSON *root = cJSON_CreateObject();
    config_to_json(root, &Config);
    char *buffer = cJSON_Print(root);

    if (buffer == NULL) {
        fprintf(console->out, "Failed to serialize current config.\n");
        return CMD_FAIL;
    }

    fputs(buffer, console->out);
    fprintf(console->out, "\n");
    cJSON_free(buffer);
    cJSON_Delete(root);
    return CMD_OK;
}

static const command_t cmd_config_list_s = {
    .command = "list",
    .help = "Print the currently loaded config.",
    .alias = 'p',
    .func = &cmd_config_list,
    .subcommands = {NULL},
};

static command_err_t cmd_config_load(int argc, char** argv, console_t* console) {
    if (argc == 0) {
        return CMD_ARG_ERR;
    }

    char path[PATH_MAX + 1] = { 0 };
    SDHelper_getRelativePath(path, PATH_MAX, argv[0], console);

    esp_err_t err = config_load_from_sd(path, &Config);
    if (err != ESP_OK) {
        fprintf(console->out, "Failed to load config: %d\n", err);
        return CMD_FAIL;
    }

    fprintf(console->out, "Config loaded from: %s\n", Config._filename);
    return CMD_OK;
}

static const command_t cmd_config_load_s = {
    .command = "load",
    .help = "Load config from a JSON file.",
    .alias = 'o',
    .func = &cmd_config_load,
    .subcommands = {NULL},
};

static command_err_t cmd_config_save(int argc, char** argv, console_t* console) {
    if (argc >= 2) {
        return CMD_ARG_ERR;
    }

    esp_err_t err;

    if (argc == 0) {
        err = config_save_to_sd(Config._filename, &Config);
    } else {
        char path[PATH_MAX + 1] = { 0 };
        SDHelper_getRelativePath(path, PATH_MAX, argv[0], console);
        err = config_save_to_sd(path, &Config);
    }

    if (err != ESP_OK) {
        fprintf(console->out, "Failed to store config: %d\n", err);
        return CMD_FAIL;
    }

    fprintf(console->out, "Config stored to: %s\n", Config._filename);
    return CMD_OK;
}

static const command_t cmd_config_save_s = {
    .command = "save",
    .help = "Save config to a JSON file.",
    .alias = 'w',
    .func = &cmd_config_save,
    .subcommands = {NULL},
};

static command_err_t cmd_config_set(int argc, char** argv, console_t* console) {
    if (argc != 2) { return CMD_ARG_ERR; }
    bool require_reboot = false;

    if (set_config_value(argv[0], argv[1], &require_reboot)) {
        fprintf(console->out, "OK\n");
        if (require_reboot) {
            // UI icon?
            fprintf(console->out, "A restart will be required for this change to take effect.\n");
        }
        return CMD_OK;
    }

    fprintf(console->out, "No such setting: %s\n", argv[0]);
    return CMD_FAIL;
}

static const command_t cmd_config_set_s = {
    .command = "set",
    .help = "Set a configuration option.",
    .alias = 's',
    .func = &cmd_config_set,
    .subcommands = {NULL},
};

static command_err_t cmd_config_get(int argc, char** argv, console_t* console) {
    if (argc != 1) { return CMD_ARG_ERR; }
    char buffer[256] = { 0 };

    if (get_config_value(argv[0], buffer, 256)) {
        fprintf(console->out, "%s\n", buffer);
        return CMD_OK;
    }

    fprintf(console->out, "No such setting: %s\n", argv[0]);
    return CMD_FAIL;
}

static const command_t cmd_config_get_s = {
    .command = "get",
    .help = "Get a configuration option.",
    .alias = 'g',
    .func = &cmd_config_get,
    .subcommands = {NULL},
};

static const command_t cmd_config_s = {
    .command = "config",
    .help = "Modify currently loaded config file.",
    .alias = 'c',
    .func = NULL,
    .subcommands = {
        &cmd_config_list_s,
        &cmd_config_load_s,
        &cmd_config_save_s,
        &cmd_config_set_s,
        &cmd_config_get_s,
        NULL,
    }
};

void commands_register_config(void) {
    console_register_command(&cmd_config_s);
}