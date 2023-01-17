#include "commands/index.h"
#include "console.h"
#include "eom-hal.h"
#include "esp_system.h"
#include "system/screenshot.h"

static command_err_t cmd_system_restart(int argc, char** argv, console_t* console) {
    if (argc != 0) {
        return CMD_ARG_ERR;
    }
    fprintf(console->out, "Device going down for restart, like, NOW!\n");
    fflush(stdout);
    esp_restart();

    return CMD_OK;
}

static const command_t cmd_system_restart_s = {
    .command = "restart",
    .help = "Restart your device",
    .alias = '\0',
    .func = &cmd_system_restart,
    .subcommands = { NULL },
};

static command_err_t cmd_system_time(int argc, char** argv, console_t* console) {
    if (argc != 0) {
        return CMD_ARG_ERR;
    }
    time_t now;
    time(&now);
    fprintf(console->out, "Time: %s", ctime(&now));
    return CMD_OK;
}

static const command_t cmd_system_time_s = {
    .command = "time",
    .help = "Get system time",
    .alias = '\0',
    .func = &cmd_system_time,
    .subcommands = { NULL },
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
        char* ptr = argv[0];
        if (ptr[0] == '#')
            ptr++;

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
    .alias = '\0',
    .func = &cmd_system_color,
    .subcommands = { NULL },
};

static command_err_t cmd_system_screenshot(int argc, char** argv, console_t* console) {
    if (argc != 0) {
        return CMD_ARG_ERR;
    }

    char path[PATH_MAX + 1] = { 0 };
    if (screenshot_save_to_file(path, PATH_MAX)) {
        console_send_file(path, console);
        return CMD_OK;
    }

    return CMD_FAIL;
}

static const command_t cmd_system_screenshot_s = {
    .command = "screenshot",
    .help = "Take a screenshot and send the file over",
    .alias = 's',
    .func = &cmd_system_screenshot,
    .subcommands = { NULL },
};



static command_err_t cmd_system_tasklist(int argc, char** argv, console_t* console) {
    size_t n = uxTaskGetNumberOfTasks();
    TaskHandle_t th = NULL;
    size_t waste = 0;

    fprintf(console->out, "ID  Task Name            High W\n");

    for (size_t i = 0; i < n; i++) {
        th = pxTaskGetNext(th);
        if (th == NULL) break;
        size_t highw = uxTaskGetStackHighWaterMark(th);
        waste += highw;
        fprintf(console->out, "%-3d %-20s %-4d\n", i, pcTaskGetName(th), highw);
    }

    fprintf(console->out, "-------------------------------\n");
    fprintf(console->out, "Wasted memory: %d\n", waste); 

    fprintf(console->out, "Heap used: %d/%d (%02f) (%d bytes free, max %d)\n", 
        heap_caps_get_total_size(MALLOC_CAP_8BIT) - heap_caps_get_free_size(MALLOC_CAP_8BIT), 
        heap_caps_get_total_size(MALLOC_CAP_8BIT),
        (1.0f - ((float) heap_caps_get_free_size(MALLOC_CAP_8BIT) / heap_caps_get_total_size(MALLOC_CAP_8BIT))) * 100,
        heap_caps_get_free_size(MALLOC_CAP_8BIT),
        heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)
    );

    return CMD_OK;
}

static const command_t cmd_system_tasklist_s = {
    .command = "tasklist",
    .help = "Dump tasks and heap information",
    .alias = 't',
    .func = &cmd_system_tasklist,
    .subcommands = { NULL },
};

static const command_t cmd_system_s = {
    .command = "system",
    .help = "System control",
    .alias = '\0',
    .func = NULL,
    .subcommands = {
        &cmd_system_restart_s,
        &cmd_system_time_s,
        &cmd_system_color_s,
        &cmd_system_screenshot_s,
        &cmd_system_tasklist_s,
        NULL,
    },
};

void commands_register_system(void) { console_register_command(&cmd_system_s); }