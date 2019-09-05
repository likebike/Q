
#define _GNU_SOURCE       // Enable strdupa.
#include <string.h>       // strcmp, strdupa
#include <unistd.h>       // access
#include <sys/stat.h>     // mkdir
#include <sys/types.h>    // mode_t
#include "dir_base_name.h"// dirname

int makedirs_full(char* path, mode_t mode) {
    // Designed to function similarly to Python's os.makedirs().
    char* parentDir = dirname(strdupa(path));  // Do NOT free() return value of dirname.
    if(strcmp(parentDir, "/") && strcmp(parentDir, ".") && access(parentDir, F_OK)) makedirs_full(parentDir, mode);
    return mkdir(path, mode);
}
int makedirs(char* path) {
    return makedirs_full(path, 0755);
}

