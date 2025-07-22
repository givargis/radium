//
// Copyright (c) Tony Givargis, 2024-2025
//
// ra_kernel.h
//

#ifndef _RA_KERNEL_H_
#define _RA_KERNEL_H_

#include <stdint.h>
#include <stddef.h>

#define RA_MIN(a,b) ( ((a) < (b)) ? (a) : (b) )
#define RA_MAX(a,b) ( ((a) > (b)) ? (a) : (b) )
#define RA_DUP(a,b) ( (0 == ((a) % (b))) ? ((a) / (b)) : ((a) / (b) + 1) )

#define RA_ARRAY_SIZE(a) ( sizeof ((a)) / sizeof ((a)[0]) )

#define RA_TRACE(s)				\
	do {					\
		ra_log("trace: %s:%d: %s",	\
		       __FILE__,		\
		       __LINE__,		\
		       (s) ? (s) : "^");	\
	} while(0)

void ra_init(int notrace, int nocolor);

uint64_t ra_time(void);

int ra_endian(void);

char *ra_strdup(const char *s);

void ra_sprintf(char *buf, size_t len, const char *format, ...);

void ra_log(const char *format, ...);

#endif // _RA_KERNEL_H_
