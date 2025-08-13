//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_kernel.h
//

#ifndef __RA_KERNEL_H__
#define __RA_KERNEL_H__

#include <assert.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>

#define RA_MIN(a, b) ( ((a) < (b)) ? (a) : (b) )
#define RA_MAX(a, b) ( ((a) > (b)) ? (a) : (b) )
#define RA_DUP(a, b) ( (0 == ((a) % (b))) ? ((a) / (b)) : ((a) / (b) + 1) )

#define RA_ARRAY_SIZE(a) ( sizeof ((a)) / sizeof ((a)[0]) )

#define RA_FREE(p)                              \
        do {                                    \
                void *p_ = (void*)(p);          \
                if (p_) {                       \
                        free(p_);               \
                }                               \
        }                                       \
        while (0)

#define RA_TRACE(s)                             \
        do {                                    \
                const char *s_ = (s);           \
                ra_printf(RA_COLOR_CYAN_BOLD,   \
                          "trace: %s:%d: %s\n", \
                          __FILE__,             \
                          __LINE__,             \
                          s_ ? s_ : "");        \
        } while(0)

typedef enum {
        RA_COLOR_BLACK,
        RA_COLOR_BLACK_BOLD,
        RA_COLOR_RED,
        RA_COLOR_RED_BOLD,
        RA_COLOR_GREEN,
        RA_COLOR_GREEN_BOLD,
        RA_COLOR_YELLOW,
        RA_COLOR_YELLOW_BOLD,
        RA_COLOR_BLUE,
        RA_COLOR_BLUE_BOLD,
        RA_COLOR_MAGENTA,
        RA_COLOR_MAGENTA_BOLD,
        RA_COLOR_CYAN,
        RA_COLOR_CYAN_BOLD,
        RA_COLOR_GRAY,
        RA_COLOR_GRAY_BOLD
} ra_color_t;

int ra_endian(void); // 0: little, 1: big

int ra_cores(void);

size_t ra_page(void);

size_t ra_memory(void);

uint64_t ra_time(void);

void *ra_malloc(size_t n);

char *ra_strdup(const char *s);

void ra_printf(ra_color_t color, const char *format, ...);

void ra_sprintf(char *buf, size_t len, const char *format, ...);

const char *ra_string(const char *format, ...);

const char *ra_pathname(const char *name);

void ra_unlink(const char *pathname);

static inline int
ra_clz(uint64_t x)
{
        return __builtin_clzll(x);
}

static inline uint64_t
ra_popcount(uint64_t x)
{
        return __builtin_popcountll(x);
}

static inline void
ra_atomic_add(volatile uint64_t *a, int64_t b)
{
        __sync_add_and_fetch(a, b);
}

static inline size_t
ra_strlen(const char *s)
{
        return s ? strlen(s) : 0;
}

static inline int
ra_is_zero(const void *buf, uint64_t len)
{
        if (*((const char *)buf)) {
                return 0;
        }
        return !memcmp(buf, ((const char *)buf) + 1, len - 1);
}

static inline void *
ra_align(void *p, size_t n)
{
        size_t r;

        if ((r = (size_t)p % n)) {
                r = n - r;
        }
        return (void *)((char *)p + r);
}

#endif // __RA_KERNEL_H__
