#ifndef __util__fs_h
#define __util__fs_h

#ifdef __cplusplus
extern "C" {
#endif

#include <dirent.h>
#include <stddef.h>
#include <stdio.h>

typedef void (*fs_read_dir_result_t)(const char* path, struct dirent* dir, void* arg);
enum fs_read_dir_flags {
    FS_READ_ALL = 0x00,
    FS_READ_NO_HIDDEN = (0x01 << 0),
    FS_READ_ABSOLUTE_PATH = (0x01 << 1),
    FS_READ_NO_FREE = (0x01 << 2),
};

size_t
fs_read_dir(const char* path, fs_read_dir_result_t result, enum fs_read_dir_flags flags, void* arg);
size_t fs_read_file(const char* path, char** data);
size_t fs_write_file(const char* path, const char* data);
const char* fs_sd_root(void);
int fs_strcmp_ext(const char* path, const char* extension);

#ifdef __cplusplus
}
#endif

#endif
