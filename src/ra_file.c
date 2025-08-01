//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_file.c
//

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ra_printf.h"
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
