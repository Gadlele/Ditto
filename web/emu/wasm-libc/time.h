#pragma once
typedef long time_t;
struct timespec { time_t tv_sec; long tv_nsec; };
#define CLOCK_MONOTONIC 1
int clock_gettime(int, struct timespec *);
typedef long clock_t;
clock_t clock(void);
#define CLOCKS_PER_SEC 1000000
