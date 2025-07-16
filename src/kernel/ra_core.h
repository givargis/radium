/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_core.h
 */

#ifndef _RA_CORE_H_
#define _RA_CORE_H_

#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#define RA__PI ( (ra__real_t)3.14159265358979323846264338327950288L )

#define RA__MIN(a,b) ( ((a) < (b)) ? (a) : (b) )
#define RA__MAX(a,b) ( ((a) > (b)) ? (a) : (b) )
#define RA__DUP(a,b) ( (0 == ((a) % (b))) ? ((a) / (b)) : ((a) / (b) + 1) )

#define RA__ARRAY_SIZE(a) ( sizeof ((a)) / sizeof ((a)[0]) )

#define RA__UNUSED(x)				\
	do {					\
		(void)(x);			\
	} while (0)

#define RA__FREE(m)				\
	do {					\
		if ((m)) {			\
			ra__free((void *)(m));	\
			(m) = NULL;		\
		}				\
	}					\
	while (0)

typedef double ra__real_t;

struct ra__complex {
	ra__real_t r;
	ra__real_t i;
};

void ra__core_init(void);

void ra__sprintf(char *buf, uint64_t len, const char *format, ...);

void ra__free(void *m);

void *ra__malloc(uint64_t n);

void *ra__realloc(void *m, uint64_t n);

char *ra__strdup(const char *s);

uint64_t ra__time(void);

int ra__endian(void);

static inline uint64_t
ra__popcount(uint64_t x)
{
	return __builtin_popcountll(x);
}

static inline void
ra__atomic_add(volatile uint64_t *a, int64_t b)
{
	__sync_add_and_fetch(a, b);
}

static inline uint64_t
ra__strlen(const char *s)
{
	return s ? (uint64_t)strlen(s) : 0;
}

static inline void *
ra__align(void *p, uint64_t n)
{
 	uint64_t r;

	if ((r = (uint64_t)p % n)) {
		r = n - r;
	}
	return (void *)((char *)p + r);
}

static inline int
ra__is_zero(const void *buf, uint64_t len)
{
	assert( buf && len );

	if (*((const char *)buf)) {
		return 0;
	}
	return !memcmp(buf, ((const char *)buf) + 1, len - 1);
}

#endif // _RA_CORE_H_
