//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_printf.c
//

#include <unistd.h>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "ra_printf.h"

void
ra_printf(ra_color_t color, const char *format, ...)
{
	int is_term;
	va_list ap;

	assert( (RA_COLOR_BLACK <= color) && (RA_COLOR_GRAY_BOLD >= color) );
	assert( format );

	if ((is_term = isatty(STDOUT_FILENO))) {
		fprintf(stderr, "\033[%dm", color / 2 + 30);
		if (0 != (color % 2)) {
			fprintf(stderr, "\033[1m");
		}
	}
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
	if (is_term) {
		fprintf(stderr, "\033[0m");
	}
	fflush(stdout);
}
