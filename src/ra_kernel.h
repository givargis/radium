//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_kernel.h
//

#ifndef __RA_KERNEL_H__
#define __RA_KERNEL_H__

#include <stdint.h>
#include <stddef.h>

#define RA_MIN(a, b) ( ((a) < (b)) ? (a) : (b) )
#define RA_MAX(a, b) ( ((a) > (b)) ? (a) : (b) )
#define RA_DUP(a, b) ( (0 == ((a) % (b))) ? ((a) / (b)) : ((a) / (b) + 1) )

#define RA_ARRAY_SIZE(a) ( sizeof ((a)) / sizeof ((a)[0]) )

typedef enum {
	RA_ENDIAN_BIG,
	RA_ENDIAN_LITTLE
} ra_endian_t;

ra_endian_t ra_endian(void);

uint64_t ra_time(void);

char *ra_strdup(const char *s);

void ra_sprintf(char *buf, size_t len, const char *format, ...);

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

#endif // __RA_KERNEL_H__
