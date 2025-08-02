//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_kernel.c
//

#include <sys/time.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "ra_logger.h"
#include "ra_kernel.h"

ra_endian_t
ra_endian(void)
{
	const uint32_t SAMPLE = 0x87654321;
	const uint8_t *probe;

	probe = (const uint8_t *)&SAMPLE;
	if ((0x21 == probe[0])) {
		if ((0x43 != probe[1]) ||
		    (0x65 != probe[2]) ||
		    (0x87 != probe[3])) {
			RA_TRACE("unsupported architecture (halting)");
			exit(-1);
		}
		return RA_ENDIAN_LITTLE;
	}
	return RA_ENDIAN_BIG;
}

uint64_t
ra_time(void)
{
	struct timeval timeval;

	if (gettimeofday(&timeval, 0)) {
		RA_TRACE("kernel failure detected");
		return 0;
	}
	return (uint64_t)timeval.tv_sec * 1000000 + (uint64_t)timeval.tv_usec;
}

char *
ra_strdup(const char *s)
{
	size_t n;
	char *p;

	n = s ? strlen(s) : 0;
	if (!(p = malloc(n + 1))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memcpy(p, s, n);
	p[n] = '\0';
	return p;
}

void
ra_sprintf(char *buf, size_t len, const char *format, ...)
{
	va_list ap;

	assert( !len || buf );
	assert( format );

	va_start(ap, format);
	if ((long long)len <= vsnprintf(buf, len, format, ap)) {
		va_end(ap);
		RA_TRACE("software bug detected (halting)");
		exit(-1);
	}
	va_end(ap);
}
