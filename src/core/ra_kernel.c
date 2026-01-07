/* Copyright (c) Tony Givargis, 2024-2026 */

#include <time.h>
#include <unistd.h>

#include "ra_hash.h"
#include "ra_kernel.h"

void
ra_kernel_init(void)
{
	if ((4  > sizeof (int)) ||
	    (8 != sizeof (long)) ||
	    (8 != sizeof (void *)) ||
	    (8 != sizeof (size_t)) ||
	    (1000000 > RAND_MAX)) {
		RA_TRACE("unsupported architecture (abort)");
		abort();
	}
}

void
ra_sleep(uint64_t us)
{
	struct timespec in, out;

	in.tv_sec = (time_t)(us / 1000000);
	in.tv_nsec = (long)(us % 1000000) * 1000;
	while (nanosleep(&in, &out)) {
		in = out;
	}
}

void
ra_unlink(const char *pathname)
{
	if (pathname && strlen(pathname)) {
		remove(pathname);
	}
}

void
ra_printf(ra_color_t color, const char *format, ...)
{
	va_list ap;
	int term;

	assert( format );

	if ((term = isatty(STDOUT_FILENO))) {
		printf("\033[%dm", color / 2 + 30);
		if (0 != (color % 2)) {
			printf("\033[1m");
			fflush(stdout);
		}
	}
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
	if (term) {
		printf("\033[0m");
		fflush(stdout);
	}
}

void
ra_sprintf(char *buf, size_t len, const char *format, ...)
{
	va_list ap;
	int rv;

	assert( (!len || buf) && format );

	va_start(ap, format);
	rv = vsnprintf(buf, len, format, ap);
	va_end(ap);
	if ((0 > rv) || (len <= (size_t)rv)) {
		RA_TRACE("integrity failure detected (abort)");
		abort();
	}
}

const char *
ra_pathname(const char *ext)
{
	const char *path;
	uint64_t tm;
	size_t len;
	char *buf;

	if (!(path = getenv("TMPDIR")) &&
	    !(path = getenv("TEMP")) &&
	    !(path = getenv("TMP"))) {
		path = "";
	}
	tm = ra_time();
	ext = ext ? ext : "";
	len = strlen(path) + strlen(ext) + 32;
	if (!(buf = malloc(len))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	ra_sprintf(buf,
		   len,
		   "%s%s%lx%s",
		   path,
		   strlen(path) ? "/" : "",
		   (unsigned long)ra_hash(&tm, sizeof (tm)),
		   ext ? ext : "~");
	return buf;
}

uint64_t
ra_time(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
		RA_TRACE("kernel failure detected");
		return 0;
	}
	return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}

size_t
ra_page(void)
{
	long page;

	if (0 >= (page = sysconf(_SC_PAGE_SIZE))) {
		RA_TRACE("unable to get kernel page size (abort)");
		abort();
	}
	return (size_t)page;
}

size_t
ra_memory(void)
{
	long page, pages;

	if ((0 >= (page = sysconf(_SC_PAGESIZE))) ||
	    (0 >= (pages = sysconf(_SC_PHYS_PAGES)))) {
		RA_TRACE("unable to get kernel memory size (abort)");
		abort();
	}
	return (size_t)page * (size_t)pages;
}

int
ra_cores(void)
{
	long cores;

	if (0 >= (cores = sysconf(_SC_NPROCESSORS_ONLN))) {
		RA_TRACE("unable to get kernel core count (abort)");
		abort();
	}
	return (int)cores;
}

int
ra_endian(void)
{
	const uint32_t SAMPLE = 0x87654321;
	const uint8_t *probe;

	probe = (const uint8_t *)&SAMPLE;
	if ((0x21 == probe[0]) &&
	    (0x43 == probe[1]) &&
	    (0x65 == probe[2]) &&
	    (0x87 == probe[3])) {
		return RA_ENDIAN_LITTLE;
	}
	if ((0x87 == probe[0]) &&
	    (0x65 == probe[1]) &&
	    (0x43 == probe[2]) &&
	    (0x21 == probe[3])) {
		return RA_ENDIAN_BIG;
	}
	RA_TRACE("unsupported architecture (abort)");
	abort();
	return -1;
}
