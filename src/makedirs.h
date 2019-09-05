#ifndef _SEB_MAKEDIRS_H
#define _SEB_MAKEDIRS_H

#include <sys/types.h> // mode_t

int makedirs(char* path);
int makedirs_full(char* path, mode_t mode);

#endif
