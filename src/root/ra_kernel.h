/**
 * Copyright (c) Tony Givargis, 2024-2025
 *
 * ra_kernel.h
 */

#ifndef _RA_KERNEL_H_
#define _RA_KERNEL_H_

#include <assert.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>

#define RA_MIN(a, b) ( ((a) < (b)) ? (a) : (b) )
#define RA_MAX(a, b) ( ((a) > (b)) ? (a) : (b) )
#define RA_DUP(a, b) ( (0 == ((a) % (b))) ? ((a) / (b)) : ((a) / (b) + 1) )

#define RA_ARRAY_SIZE(a) ( sizeof ((a)) / sizeof ((a)[0]) )

#define RA_TRACE(s)				\
	do {					\
		ra_log("trace: %s:%d: %s",	\
		       __FILE__,		\
		       __LINE__,		\
		       (s) ? (s) : "^");	\
	} while(0)

typedef enum {
	RA_ENDIAN_LITTLE,
	RA_ENDIAN_BIG
} ra_endian_t;

typedef enum {
	RA_DIRECTORY_TEMP,
	RA_DIRECTORY_DATA
} ra_directory_t;

void ra_init(int notrace, int nocolor);

void ra_wait(void);

uint64_t ra_time(void);

ra_endian_t ra_endian(void);

char *ra_strdup(const char *s);

void ra_sprintf(char *buf, size_t len, const char *format, ...);

void ra_log(const char *format, ...);

const char *ra_pathname(ra_directory_t type, const char *name);

#endif // _RA_KERNEL_H_
