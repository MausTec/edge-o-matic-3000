#include "commands/index.h"
#include "console.h"
#include "eom-hal.h"
#include "maus_bus.h"

static void _print_device_name(maus_bus_device_t *device, maus_bus_address_t address, void* ptr) {
    printf("Found device: %s\n", device->product_name);
}

static command_err_t cmd_bus_scan(int argc, char** argv, console_t* console) {
    if (argc != 0) {
        return CMD_ARG_ERR;
    }

    fprintf(console->out, "Scanning I2C bus...\n");
    
    size_t devices = maus_bus_scan_bus_full(&_print_device_name, NULL);
    fprintf(console->out, "Found %d devices.\n", devices);
    maus_bus_free_device_scan();

    return CMD_OK;
}

static const command_t cmd_bus_scan_s = {
    .command = "scan",
    .help = "Scan the I2C bus for devices",
    .alias = 's',
    .func = &cmd_bus_scan,
    .subcommands = { NULL },
};

static command_err_t cmd_bus_read(int argc, char** argv, console_t* console) {
    if (argc < 2 || argc > 3) {
        return CMD_ARG_ERR;
    }

    uint8_t address = strtoul(argv[0], NULL, 16);
    size_t len = strtoul(argv[argc - 1], NULL, 0);
    uint8_t *result = malloc(len);

    if (result == NULL) {
        return CMD_FAIL;
    }

    if (argc == 2) {
        fprintf(console->out, "Reading %d bytes from 0x%02X...\n", len, address);
        eom_hal_accessory_master_read(address, result, len);
    } else {
        uint8_t reg = strtoul(argv[1], NULL, 16);
        fprintf(console->out, "Reading %d bytes from 0x%02X @ 0x%02x...\n", len, address, reg);
        eom_hal_accessory_master_read_register(address, reg, result, len);
    }

    for (size_t i = 0; i < len; i++) {
        if (i % 0x10 == 0) {
            if (i > 0) fprintf(console->out, "\n");
            fprintf(console->out, "%02x - ", i);
        }
        fprintf(console->out, "%02x ", result[i]);
    }
    fprintf(console->out, "\n");

    free(result);
    return CMD_OK;
}

static const command_t cmd_bus_read_s = {
    .command = "read",
    .help = "Read n bytes from address",
    .alias = 'r',
    .func = &cmd_bus_read,
    .subcommands = { NULL },
};

static command_err_t cmd_bus_write(int argc, char** argv, console_t* console) {
    if (argc < 2) {
        return CMD_ARG_ERR;
    }

    uint8_t address = strtoul(argv[0], NULL, 16);
    size_t len = argc - 1;
    uint8_t *buffer = malloc(len);

    if (buffer == NULL) {
        return CMD_FAIL;
    }

    for (size_t i = 0; i < len; i++) {
        buffer[i] = strtoul(argv[i + 1], NULL, 16);
    }

    fprintf(console->out, "Writing %d bytes to 0x%02X...\n", len, address);
    eom_hal_accessory_master_write(address, buffer, len);

    free(buffer);
    return CMD_OK;
}

static const command_t cmd_bus_write_s = {
    .command = "write",
    .help = "Write a sequence of bytes to an address",
    .alias = 'w',
    .func = &cmd_bus_write,
    .subcommands = { NULL },
};

static command_err_t cmd_bus_write_str(int argc, char** argv, console_t* console) {
    if (argc != 3) {
        return CMD_ARG_ERR;
    }

    uint8_t address = strtoul(argv[0], NULL, 16);
    size_t len = strlen(argv[2]) + 2;
    uint8_t *buffer = malloc(len);

    if (buffer == NULL) {
        return CMD_FAIL;
    }

    buffer[0] = strtoul(argv[1], NULL, 16);
    memcpy(buffer + 1, argv[2], strlen(argv[2]) + 1);

    fprintf(console->out, "Writing %d bytes to 0x%02X...\n", len, address);
    eom_hal_accessory_master_write(address, buffer, len);

    free(buffer);
    return CMD_OK;
}

static const command_t cmd_bus_write_str_s = {
    .command = "writestr",
    .help = "Write a string to an address",
    .alias = 's',
    .func = &cmd_bus_write_str,
    .subcommands = { NULL },
};

static const command_t cmd_bus_s = {
    .command = "bus",
    .help = "Bus / Accessory Control",
    .alias = '\0',
    .func = NULL,
    .subcommands = {
        &cmd_bus_scan_s,
        &cmd_bus_read_s,
        &cmd_bus_write_s,
        &cmd_bus_write_str_s,
        NULL,
    },
};

void commands_register_bus(void) { console_register_command(&cmd_bus_s); }