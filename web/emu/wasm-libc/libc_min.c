/* the C library functions the device code needs in a WebAssembly build without one (web/emu/build_wasm.sh) */
#include <stddef.h>
#include <stdint.h>
void *memset(void *d, int c, size_t n) { unsigned char *p = d; while (n--) *p++ = (unsigned char)c; return d; }
void *memcpy(void *d, const void *s, size_t n) { unsigned char *p = d; const unsigned char *q = s; while (n--) *p++ = *q++; return d; }
void *memmove(void *d, const void *s, size_t n)
{
    unsigned char *p = d; const unsigned char *q = s;
    if (p < q) while (n--) *p++ = *q++;
    else { p += n; q += n; while (n--) *--p = *--q; }
    return d;
}
int memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *p = a, *q = b;
    for (; n; n--, p++, q++) if (*p != *q) return *p - *q;
    return 0;
}
size_t strlen(const char *s) { size_t n = 0; while (s[n]) n++; return n; }
int strcmp(const char *a, const char *b) { while (*a && *a == *b) a++, b++; return (unsigned char)*a - (unsigned char)*b; }
char *strchr(const char *s, int c) { for (; *s; s++) if (*s == (char)c) return (char *)s; return c ? 0 : (char *)s; }
int abs(int x) { return x < 0 ? -x : x; }
int atoi(const char *s) { int v = 0, n = 0; while (*s == ' ') s++; if (*s == '-') n = 1, s++; while (*s >= '0' && *s <= '9') v = v * 10 + *s++ - '0'; return n ? -v : v; }
/* (the page's worklet calls it, as an Emscripten standalone build exports it: nothing to set up here) */
__attribute__((export_name("_initialize"), visibility("default"))) void wasm_initialize(void) {}
