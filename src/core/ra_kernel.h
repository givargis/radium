/* Copyright (c) Tony Givargis, 2024-2026 */

#ifndef __RA_KERNEL_H__
#define __RA_KERNEL_H__

#include <math.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#define RA_ENDIAN_BIG    0
#define RA_ENDIAN_LITTLE 1

#define RA_MIN(a, b) ( ((a) < (b)) ? (a) : (b) )
#define RA_MAX(a, b) ( ((a) > (b)) ? (a) : (b) )
#define RA_DUP(a, b) ( (0 == ((a) % (b))) ? ((a) / (b)) : ((a) / (b) + 1) )

#define RA_ARRAY_SIZE(a) ( sizeof ((a)) / sizeof ((a)[0]) )

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
		ra_printf(RA_COLOR_YELLOW_BOLD,	\
			  "trace: ");		\
		ra_printf(RA_COLOR_BLACK,	\
			  "%s:%d: %s\n",	\
			  __FILE__,		\
			  __LINE__,		\
			  (s) ? (s) : "");	\
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

void ra_kernel_init(void);

void ra_sleep(uint64_t us);

void ra_unlink(const char *pathname);

void ra_trace(const char *format, ...);

void ra_printf(ra_color_t color, const char *format, ...);

void ra_sprintf(char *buf, size_t len, const char *format, ...);

const char *ra_pathname(const char *ext); /* caller free */

uint64_t ra_time(void);

size_t ra_page(void);

size_t ra_memory(void);

int ra_cores(void);

int ra_endian(void);

static __attribute__((unused)) int
ra_is_zero(const void *buf, uint64_t len)
{
	if (*((const char *)buf)) {
		return 0;
	}
	return !memcmp(buf, ((const char *)buf) + 1, len - 1);
}

static __attribute__((unused)) void *
ra_align(void *p, size_t n)
{
	size_t r;

	if ((r = (size_t)p % n)) {
		r = n - r;
	}
	return (void *)((char *)p + r);
}

#endif /* __RA_KERNEL_H__ */
