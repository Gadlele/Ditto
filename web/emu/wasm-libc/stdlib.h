#pragma once
#include <stddef.h>
void *malloc(size_t);
void *calloc(size_t, size_t);
void free(void *);
int atoi(const char *);
long strtol(const char *, char **, int);
char *getenv(const char *);
int abs(int);
void exit(int);
void qsort(void *, size_t, size_t, int (*)(const void *, const void *));
