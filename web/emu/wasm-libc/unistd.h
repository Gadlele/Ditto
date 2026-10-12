#pragma once
#include <stddef.h>
typedef int pid_t;
typedef long ssize_t;
int fork(void); int usleep(unsigned); int unlink(const char *); int pipe(int *); ssize_t write(int, const void *, size_t);
ssize_t read(int, void *, size_t); int close(int); void _exit(int);
