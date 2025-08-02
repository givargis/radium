//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_file.c
//

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ra_kernel.h"
#include "ra_logger.h"
#include "ra_file.h"

char *
ra_file_read(const char *pathname)
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
		free(s);
		fclose(file);
		RA_TRACE("file read failed");
		return NULL;
	}
	memset(s + size, 0, PAD);
	fclose(file);
	return s;
}

int
ra_file_write(const char *pathname, const char *s)
{
	FILE *file;

	assert( pathname && strlen(pathname) );
	assert( s );

	if (!(file = fopen(pathname, "w"))) {
		RA_TRACE("unable to open file");
		return -1;
	}
	if (strlen(s) && (1 != fwrite(s, strlen(s), 1, file))) {
		fclose(file);
		RA_TRACE("file write failed");
		return -1;
	}
	fclose(file);
	return 0;
}

void
ra_file_unlink(const char *pathname)
{
	if (pathname) {
		(void)remove(pathname);
	}
}

int
ra_file_test(void)
{
	const char *pathname, *s;

	// zero-byte write/read

	if (!(pathname = ra_temp_pathname(NULL)) ||
	    ra_file_write(pathname, "")) {
		free((void *)pathname);
		RA_TRACE("^");
		return -1;
	}
	if (!(s = ra_file_read(pathname)) || strlen(s)) {
		ra_file_unlink(pathname);
		free((void *)pathname);
		free((void *)s);
		RA_TRACE("software bug detected");
		return -1;
	}
	free((void *)s);

	// single-byte write/read

	if (ra_file_write(pathname, "x")) {
		free((void *)pathname);
		RA_TRACE("^");
		return -1;
	}
	if (!(s = ra_file_read(pathname)) ||
	    (1 != strlen(s)) ||
	    ('x' != (*s))) {
		ra_file_unlink(pathname);
		free((void *)pathname);
		free((void *)s);
		RA_TRACE("software bug detected");
		return -1;
	}
	free((void *)s);

	// multi-byte write

	if (ra_file_write(pathname, pathname)) {
		free((void *)pathname);
		RA_TRACE("^");
		return -1;
	}
	if (!(s = ra_file_read(pathname)) ||
	    (strlen(pathname) != strlen(s)) ||
	    strcmp(pathname, s)) {
		ra_file_unlink(pathname);
		free((void *)pathname);
		free((void *)s);
		RA_TRACE("software bug detected");
		return -1;
	}
	ra_file_unlink(pathname);
	free((void *)pathname);
	free((void *)s);
	return 0;
}
