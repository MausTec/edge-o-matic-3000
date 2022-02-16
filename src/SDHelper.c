#include "SDHelper.h"
#include <string.h>
#include "cJSON.h"
#include "eom-hal.h"

size_t SDHelper_join(char* buf, size_t len, const char* path) {
    size_t size = 0;
    char* saveptr = NULL,
        * token = NULL,
        tmppath[PATH_MAX + 1];

    strlcpy(tmppath, path, PATH_MAX);

    // Remove trailing slash from buf
    if (buf[0] != '\0' && buf[strlen(buf) - 1] == '/') {
        buf[strlen(buf) - 1] = '\0';
    }

    token = strtok_r(tmppath, "/\\", &saveptr);
    while (token != NULL) {
        if (!strcmp(token, "..")) {
            char *parent = strrchr(buf, '/');
            if (parent) *parent = '\0';
            else buf[0] = '\0';
        } else if (!strcmp(token, ".")) {
            // ... do nothing
        } else {
            strlcat(buf, "/", len);
            strlcat(buf, token, len);
        }
        token = strtok_r(NULL, "/\\", &saveptr);
    }

    if (buf[0] == '\0') {
        buf[0] = '/';
        buf[1] = '\0';
    }

    return strlen(buf);
}

// FIXME: Apparently this fails to link. I think it's something weird with my setup.
void SDHelper_printDirectoryJson(DIR* dir, cJSON* files) {
    // while (true) {
    //   File entry = dir.openNextFile();
    //   if (! entry) {
    //     break;
    //   }

    //   JsonArray list = files.to<JsonArray>();
    //   JsonObject file = list.createNestedObject();
    //   file["name"] = entry.name();
    //   file["size"] = entry.size();
    //   file["dir"]  = entry.isDirectory();

    //   entry.close();
    // }
}

void SDHelper_printDirectory(DIR* dir, int numTabs, char* out, size_t len) {
    // while (true) {
    //   File entry =  dir.openNextFile();
    //   if (! entry) {
    //     // no more files
    //     break;
    //   }
    //   for (uint8_t i = 0; i < numTabs; i++) {
    //     out += "\t";
    //   }
    //   out += std::to_string(entry.name());
    //   if (entry.isDirectory()) {
    //     out += "/\n";
    //     printDirectory(entry, numTabs + 1, out);
    //   } else {
    //     // files have sizes, directories do not
    //     out += "\t\t";
    //     out += std::to_string(entry.size()) + "\n";
    //   }
    //   entry.close();
    // }
}

void SDHelper_logDirectory(const char* path) {
    DIR* d;
    struct dirent* dir;
    d = opendir(path);
    if (d) {
        printf("Directory contents of %s:\n\n", path);
        while ((dir = readdir(d)) != NULL) {
            printf("%02x\t%s\n", dir->d_type, dir->d_name);
        }
        closedir(d);
    } else {
        printf("Directory not found.\n");
    }
}

void SDHelper_logFile(const char* path) {
    FILE* f;
    f = fopen(path, "r");

    if (f) {
        char c;
        while ((c = fgetc(f)) != 0xFF) {
            printf("%c", c);
        }
        printf("\n");
    } else {
        printf("File not found.\n");
    }
}

void SDHelper_printFile(FILE* file, char* out, size_t len) {
    // while (file->available()) {
    //   out += std::to_string((char)file.read());
    // }
    // file.close();
}

size_t SDHelper_getAbsolutePath(char* out, size_t len, const char* path) {
    if (out == NULL || len == 0) {
        return strlen(eom_hal_get_sd_mount_point()) + strlen(path) + 1;
    } else {
        strlcpy(out, eom_hal_get_sd_mount_point(), len);
        return SDHelper_join(out, len, path);
    }
}