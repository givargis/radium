/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_core.c
 */

#include <sys/time.h>

#include "ra_error.h"
#include "ra_core.h"

void
ra__core_init(void)
{
	if ((1 != sizeof (char)) ||
	    (4 != sizeof (float)) ||
	    (8 != sizeof (double)) ||
	    (8 != sizeof (size_t)) ||
	    (8 != sizeof (void *)) ||
	    (8 != sizeof (void (*)(void)))) {
		RA__ERROR_HALT(RA__ERROR_ARCHITECTURE);
	}
}

void
ra__sprintf(char *buf, uint64_t len, const char *format, ...)
{
	va_list ap;

	assert( (!len || buf) );
	assert( format );

	va_start(ap, format);
	if ((int)len <= vsnprintf(buf, len, format, ap)) {
		va_end(ap);
		RA__ERROR_HALT(RA__ERROR_SOFTWARE);
	}
	va_end(ap);
}

void
ra__free(void *m)
{
	if (m) {
		free(m);
	}
}

void *
ra__malloc(uint64_t n)
{
	void *p;

	assert( n );

	if (!(p = malloc(n))) {
		RA__ERROR_TRACE(RA__ERROR_MEMORY);
		return NULL;
	}
	return p;
}

void *
ra__realloc(void *m, uint64_t n)
{
	void *p;

	assert( n );

	if (!(p = realloc(m, n))) {
		RA__ERROR_TRACE(RA__ERROR_MEMORY);
		return NULL;
	}
	return p;
}

char *
ra__strdup(const char *s)
{
	uint64_t n;
	char *p;

	n = ra__strlen(s);
	if (!(p = ra__malloc(n + 1))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memcpy(p, s, n);
	p[n] = '\0';
	return p;
}

uint64_t
ra__time(void)
{
	struct timeval timeval;

	if (gettimeofday(&timeval, 0)) {
		RA__ERROR_HALT(RA__ERROR_KERNEL);
		return 0;
	}
	return (uint64_t)timeval.tv_sec * 1000000 + (uint64_t)timeval.tv_usec;
}

int
ra__endian(void)
{
	uint32_t sample;
	uint8_t *probe;

	sample = 0x87654321;
	probe = (uint8_t *)&sample;
	if ((0x21 == probe[0])) {
		if ((0x43 != probe[1]) ||
		    (0x65 != probe[2]) ||
		    (0x87 != probe[3])) {
			RA__ERROR_HALT(RA__ERROR_ARCHITECTURE);
 		}
		return 0; // little
	}
	return 1; // big
}
