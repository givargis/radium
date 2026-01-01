/* Copyright (c) Tony Givargis, 2024-2026 */

#ifndef __RA_KERNEL_H__
#define __RA_KERNEL_H__

#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#define RA_ENDIAN_BIG    0
#define RA_ENDIAN_LITTLE 1

#define RA_MIN(a, b) ( ((a) < (b)) ? (a) : (b) )
#define RA_MAX(a, b) ( ((a) > (b)) ? (a) : (b) )
#define RA_DUP(a, b) ( (0 == ((a) % (b))) ? ((a) / (b)) : ((a) / (b) + 1) )

#define RA_ARRAY_SIZE(a) ( sizeof ((a)) / sizeof ((a)[0]) )

#define RA_STATIC static __attribute__((unused))

#define RA_FREE(p)				\
	do {					\
		if (p) {			\
			free((void *)(p));	\
			(p) = NULL;		\
		}				\
	}					\
	while (0)

#define RA_TRACE(s)				\
	do {					\
		ra_trace("%s:%d: %s",		\
			 __FILE__,		\
			 __LINE__,		\
			 (s) ? (s) : "");	\
	} while(0)

void ra_kernel_init(int notrace);

void ra_unlink(const char *pathname);

void ra_trace(const char *format, ...);

void ra_error(const char *format, ...);

void ra_sprintf(char *buf, size_t len, const char *format, ...);

char *ra_textfile_read(const char *pathname);

int ra_textfile_write(const char *pathname, const char *s);

uint64_t ra_time(void);

size_t ra_page(void);

size_t ra_memory(void);

int ra_cores(void);

int ra_endian(void);

RA_STATIC int
ra_clz(uint64_t x)
{
	return __builtin_clzll(x);
}

RA_STATIC uint64_t
ra_popcount(uint64_t x)
{
	return __builtin_popcountll(x);
}

RA_STATIC int
ra_is_zero(const void *buf, uint64_t len)
{
	if (*((const char *)buf)) {
		return 0;
	}
	return !memcmp(buf, ((const char *)buf) + 1, len - 1);
}

RA_STATIC void *
ra_align(void *p, size_t n)
{
	size_t r;

	if ((r = (size_t)p % n)) {
		r = n - r;
	}
	return (void *)((char *)p + r);
}

#endif /* __RA_KERNEL_H__ */
