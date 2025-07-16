/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_log.c
 */

#include "ra_spinlock.h"
#include "ra_core.h"
#include "ra_term.h"
#include "ra_log.h"

static int _notrace_;

void
ra__log_init(int notrace)
{
	_notrace_ = notrace ? 1 : 0;
}

void
ra__log(const char *format, ...)
{
	static ra__spinlock_t lock;
	struct tm *tm;
	va_list ap;
	time_t t;

	assert( format );

	ra__spinlock_lock(&lock);

	// M.E. ->

	t = time(NULL);
	tm = gmtime(&t);
	if (strncmp(format, "trace:", 6) || !_notrace_) {
		ra__term_bold();
		printf("[%02d-%02d-%d %02d:%02d:%02d]: ",
		       tm->tm_mon + 1,
		       tm->tm_mday,
		       tm->tm_year + 1900,
		       tm->tm_hour,
		       tm->tm_min,
		       tm->tm_sec);
		if (!strncmp(format, "trace:", 6)) {
			format += 6;
			ra__term_color(RA__TERM_COLOR_CYAN);
			printf("trace:");
		}
		else if (!strncmp(format, "error:", 6)) {
			format += 6;
			ra__term_color(RA__TERM_COLOR_RED);
			printf("error:");
		}
		else if (!strncmp(format, "warning:", 8)) {
			format += 8;
			ra__term_color(RA__TERM_COLOR_YELLOW);
			printf("warning:");
		}
		else if (!strncmp(format, "info:", 5)) {
			format += 5;
			ra__term_color(RA__TERM_COLOR_BLUE);
			printf("info:");
		}
		ra__term_reset();
		ra__term_bold();
		va_start(ap, format);
		vprintf(format, ap);
		va_end(ap);
		printf("\n");
		ra__term_reset();
		fflush(stdout);
	}

	// M.E. <-

	ra__spinlock_unlock(&lock);
}
