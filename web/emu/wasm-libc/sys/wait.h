#pragma once
int wait(int *); int waitpid(int, int *, int);
#define WEXITSTATUS(s) (((s) >> 8) & 255)
#define WIFEXITED(s) 1
