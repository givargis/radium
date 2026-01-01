/* Copyright (c) Tony Givargis, 2024-2026 */

#include <time.h>
#include <unistd.h>

#include "ra_kernel.h"

void
ra_kernel_init(void)
{
	if ((8 != sizeof (long)) ||
	    (8 != sizeof (void *)) ||
	    (8 != sizeof (size_t))) {
		RA_TRACE("unsupported architecture (abort)");
		abort();
	}
}

void
ra_unlink(const char *pathname)
{
	if (pathname && strlen(pathname)) {
		(void)remove(pathname);
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
		}
	}
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
	if (term) {
		printf("\033[0m");
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
		RA_TRACE("software bug detected (abort)");
		abort();
	}
}

char *
ra_textfile_read(const char *pathname)
{
	const size_t PAD = 4;
	FILE *file;
	long size;
	char *s;

	assert( pathname && strlen(pathname) );

	if (!(file = fopen(pathname, "r"))) {
		RA_TRACE("unable to open file");
		return NULL;
	}
	if (fseek(file, 0, SEEK_END) ||
	    (0 > (size = ftell(file))) ||
	    fseek(file, 0, SEEK_SET)) {
		fclose(file);
		RA_TRACE("unable to get file stat");
		return NULL;
	}
	if (!(s = malloc(size + PAD))) {
		fclose(file);
		RA_TRACE("out of memory");
		return NULL;
	}
	if (size && (1 != fread(s, size, 1, file))) {
		RA_FREE(s);
		fclose(file);
		RA_TRACE("file read failed");
		return NULL;
	}
	memset(s + size, 0, PAD);
	fclose(file);
	return s;
}

int
ra_textfile_write(const char *pathname, const char *s)
{
	size_t size;
	FILE *file;

	assert( pathname && strlen(pathname) && s );

	size = strlen(s);
	if (!(file = fopen(pathname, "w"))) {
		RA_TRACE("unable to open file");
		return -1;
	}
	if (size && (1 != fwrite(s, size, 1, file))) {
		fclose(file);
		RA_TRACE("file write failed");
		return -1;
	}
	fclose(file);
	return 0;
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
		RA_TRACE("unable to get kernel page size (abort)");
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
