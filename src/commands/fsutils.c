#include "commands/index.h"
#include "console.h"

#include <stdio.h>
#include <sys/stat.h>
#include "SDHelper.h"
#include "esp_vfs.h"
#include <errno.h>

static void _mkpath(char* path, const char* argv, console_t* console) {
    if (argv != NULL) {
        if (argv[0] == '/') {
            SDHelper_getAbsolutePath(path, PATH_MAX, argv);
        } else {
            SDHelper_join(path, PATH_MAX, console->cwd);
            SDHelper_join(path, PATH_MAX, argv);
        }
    } else {
        SDHelper_join(path, PATH_MAX, console->cwd);
    }
}

static bool _isfile(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static bool _isdir(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static command_err_t cmd_cd(int argc, char** argv, console_t* console) {
    if (argc == 0) {
        return CMD_ARG_ERR;
    }

    char path[PATH_MAX + 1] = { 0 };
    _mkpath(path, argv[0], console);

    if (_isdir(path)) {
        strncpy(console->cwd, path, PATH_MAX);
        return CMD_OK;
    } else {
        fprintf(console->out, "No such directory: %s\n", argv[0]);
        return CMD_FAIL;
    }
}

const command_t cmd_cd_s = {
    .command = "cd",
    .help = "Change working directory",
    .alias = '.',
    .func = &cmd_cd,
    .subcommands = { NULL },
};

static command_err_t cmd_ls(int argc, char** argv, console_t* console) {
    char path[PATH_MAX + 1] = { 0 };
    _mkpath(path, argc > 0 ? argv[0] : NULL, console);

    DIR* d;
    struct dirent* dir;
    d = opendir(path);

    if (d) {
        fprintf(console->out, "Directory contents of %s:\n\n", path);
        while ((dir = readdir(d)) != NULL) {
            char dt = '?';

            switch (dir->d_type) {
            case DT_REG: dt = '-'; break;
            case DT_DIR: dt = 'd'; break;
            default: dt = '?'; break;
            }

            fprintf(console->out, "%c  %s\n", dt, dir->d_name);
        }
        closedir(d);
        return CMD_OK;
    } else {
        fprintf(console->out, "Directory not found.\n");
        return CMD_FAIL;
    }
}

const command_t cmd_ls_s = {
    .command = "ls",
    .help = "List directory contents",
    .alias = NULL,
    .func = &cmd_ls,
    .subcommands = { NULL },
};

static command_err_t cmd_pwd(int argc, char** argv, console_t* console) {
    fprintf(console->out, "%s\n", console->cwd);
    return CMD_OK;
}

const command_t cmd_pwd_s = {
    .command = "pwd",
    .help = "Print working directory",
    .alias = NULL,
    .func = &cmd_pwd,
    .subcommands = { NULL },
};

static command_err_t cmd_mkdir(int argc, char** argv, console_t* console) {
    if (argc == 0) {
        return CMD_ARG_ERR;
    }

    bool parents = !strcmp(argv[0], "-p");
    const char* new_path = argv[argc - 1];
    char path[PATH_MAX + 1] = { 0 };
    _mkpath(path, new_path, console);

    if (parents) {
        char* token = NULL;
        char* savptr = NULL;

        // Iterate through our path, building each directory level and ignoring existing dirs.
        token = strtok_r(path, "/", &savptr);
        while (token != NULL) {
            if (mkdir(path, S_IRWXU) != 0 && errno != EEXIST) {
                fprintf(console->out, "Failed to create directory: %s\n", strerror(errno));
                return CMD_FAIL;
            }

            // Replace the removed token to continue building our path:
            if (savptr != NULL) {
                savptr[-1] = '/';
            }

            token = strtok_r(NULL, "/", &savptr);
        }
        return CMD_OK;
    } else {
        if (mkdir(path, S_IRWXU) != 0) {
            if (errno == EEXIST) {
                fprintf(console->out, "Directory already exists.\n");
            } else {
                fprintf(console->out, "Failed to create directory: %s\n", strerror(errno));
            }
            return CMD_FAIL;
        } else {
            return CMD_OK;
        }
    }
}

const command_t cmd_mkdir_s = {
    .command = "mkdir",
    .help = "Make a new directory",
    .alias = NULL,
    .func = &cmd_mkdir,
    .subcommands = { NULL },
};

static command_err_t cmd_rm(int argc, char** argv, console_t* console) {
    if (argc == 0) {
        return CMD_ARG_ERR;
    }

    bool recurse = !strcmp(argv[0], "-r");
    char path[PATH_MAX + 1] = { 0 };
    _mkpath(path, argv[argc - 1], console);

    if (_isdir(path)) {
        // TODO: Handle recurse here
        if (rmdir(path) != 0) {
            fprintf(console->out, "Error removing directory: %s\n", strerror(errno));
            return CMD_FAIL;
        }
    } else if (_isfile(path)) {
        if (unlink(path) != 0) {
            fprintf(console->out, "Error removing file: %s\n", strerror(errno));
            return CMD_FAIL;
        }
    } else {
        fprintf(console->out, "No such file or directory: %s\n", path);
        return CMD_FAIL;
    }

    return CMD_OK;
}

const command_t cmd_rm_s = {
    .command = "rm",
    .help = "Remove a regular file or directory",
    .alias = NULL,
    .func = &cmd_rm,
    .subcommands = { NULL },
};

static command_err_t cmd_cat(int argc, char** argv, console_t* console) {
    if (argc == 0) { return CMD_ARG_ERR; }

    char path[PATH_MAX + 1] = { 0 };
    bool binary = !strcmp(argv[0], "-b");
    SDHelper_getRelativePath(path, PATH_MAX, argv[argc - 1], console);

    FILE *f = fopen(path, binary ? "rb" : "r");
    if (!f) {
        fprintf(console->out, "No such file or directory: %s\n", path);
        return CMD_FAIL;
    }

    int c;
    size_t idx = 0;
    while ((c = fgetc(f)) != EOF) {
        fprintf(console->out, binary ? "%02x" : "%c", c);
        if (binary && idx++ > 32) {
            fprintf(console->out, "\n");
            idx = 0;
        }
    }

    fprintf(console->out, "\n");
    fclose(f);
    return CMD_OK;
}

static const command_t cmd_cat_s = {
    .command = "cat",
    .help = "Print the contents of a file to the terminal",
    .alias = NULL,
    .func = &cmd_cat,
    .subcommands = {NULL},
};

void commands_register_fsutils(void) {
    console_register_command(&cmd_cd_s);
    console_register_command(&cmd_ls_s);
    console_register_command(&cmd_pwd_s);
    console_register_command(&cmd_mkdir_s);
    console_register_command(&cmd_rm_s);
    console_register_command(&cmd_cat_s);
}
