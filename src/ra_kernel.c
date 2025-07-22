//
// Copyright (c) Tony Givargis, 2024-2025
//
// ra_kernel.c
//

#include <sys/time.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "ra_kernel.h"

static int _notrace_;
static int _nocolor_;

enum { BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, GRAY };

static void
color(int color)
{
	if (!_nocolor_) {
		fprintf(stderr, "\033[%dm", 30 + color);
		fprintf(stderr, "\033[1m");
	}
}

static void
reset(void)
{
	if (!_nocolor_) {
		fprintf(stderr, "\033[0m");
	}
}

void
ra_init(int notrace, int nocolor)
{
	_notrace_ = notrace ? 1 : 0;
	_nocolor_ = nocolor ? 1 : 0;
}

uint64_t
ra_time(void)
{
	struct timeval timeval;

	if (gettimeofday(&timeval, 0)) {
		RA_TRACE("kernel");
		return 0;
	}
	return (uint64_t)timeval.tv_sec * 1000000 + (uint64_t)timeval.tv_usec;
}

int
ra_endian(void)
{
	const uint32_t SAMPLE = 0x87654321;
	const uint8_t *probe;

	probe = (const uint8_t *)&SAMPLE;
	if ((0x21 == probe[0])) {
		if ((0x43 != probe[1]) ||
		    (0x65 != probe[2]) ||
		    (0x87 != probe[3])) {
			RA_TRACE("architecture (halting)");
			exit(-1);
		}
		return 0; // little
	}
	return 1; // big
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

	assert( (!len || buf) );
	assert( format );

	va_start(ap, format);
	if ((long long)len <= vsnprintf(buf, len, format, ap)) {
		va_end(ap);
		RA_TRACE("software (halting)");
		exit(-1);
	}
	va_end(ap);
}

void
ra_log(const char *format, ...)
{
	va_list ap;

	assert( format );

	if (strncmp(format, "trace:", 6) || !_notrace_) {
		if (!strncmp(format, "trace:", 6)) {
			format += 6;
			color(CYAN);
			printf("trace:");
		}
		else if (!strncmp(format, "error:", 6)) {
			format += 6;
			color(RED);
			printf("error:");
		}
		else if (!strncmp(format, "info:", 5)) {
			format += 5;
			color(BLUE);
			printf("info:");
		}
		va_start(ap, format);
		vprintf(format, ap);
		va_end(ap);
		printf("\n");
		reset();
		fflush(stdout);
	}
}
