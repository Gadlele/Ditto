/* Golana: the few stdio declarations tests/hostsim.c names, for a WebAssembly build without a C library
 * (web/emu/build_wasm.sh). Only the device code is linked; the host tests that call these are left out. */
#pragma once
#include <stddef.h>
#include <stdarg.h>
typedef struct FILE FILE;
extern FILE *stderr, *stdout;
int printf(const char *, ...);
int fprintf(FILE *, const char *, ...);
int snprintf(char *, size_t, const char *, ...);
FILE *fopen(const char *, const char *);
int fclose(FILE *);
size_t fwrite(const void *, size_t, size_t, FILE *);
size_t fread(void *, size_t, size_t, FILE *);
int fflush(FILE *);
int puts(const char *);
int putchar(int);
int fputs(const char *, FILE *);
int fseek(FILE *, long, int);
long ftell(FILE *);
int sscanf(const char *, const char *, ...);
int remove(const char *);
int vsnprintf(char *, size_t, const char *, va_list);
