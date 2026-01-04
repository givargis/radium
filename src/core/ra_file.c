/* Copyright (c) Tony Givargis, 2024-2026 */

#include <sys/stat.h>
#include "ra_file.h"

void *
ra_file_read(const char *pathname, size_t *len_)
{
	struct stat st;
	FILE *file;
	size_t len;
	char *buf;

	if (!pathname || !*pathname || !len_) {
		RA_TRACE("invalid arguments");
		return NULL;
	}
	if (!(file = fopen(pathname, "rb"))) {
		RA_TRACE("unable to open file");
		return NULL;
	}
	if (fstat(fileno(file), &st)) {
		fclose(file);
		RA_TRACE("unable to stat file");
		return NULL;
	}
	if (!S_ISREG(st.st_mode)) {
		fclose(file);
		RA_TRACE("not a regular file");
		return NULL;
	}
	if ((0 > st.st_size) || ((SIZE_MAX - 1) < (size_t)st.st_size)) {
		fclose(file);
		RA_TRACE("file too large");
		return NULL;
	}
	len = (size_t)st.st_size;
	if (!(buf = malloc(len + 1))) {
		fclose(file);
		RA_TRACE("out of memory");
		return NULL;
	}
	if (len != fread(buf, 1, len, file)) {
		RA_FREE(buf);
		fclose(file);
		RA_TRACE("file read failed");
		return NULL;
	}
	fclose(file);
	buf[len] = '\0';
	(*len_) = (size_t)len;
	return buf;
}

int
ra_file_write(const char *pathname, const void *buf, size_t len)
{
	FILE *file;

	if (!pathname || !*pathname || (len && !buf)) {
		RA_TRACE("invalid arguments");
		return -1;
	}
	if (!(file = fopen(pathname, "wb"))) {
		RA_TRACE("unable to open file");
		return -1;
	}
	if (len != fwrite(buf, 1, len, file)) {
		fclose(file);
		ra_unlink(pathname);
		RA_TRACE("file write failed");
		return -1;
	}
	if (fclose(file)) {
		ra_unlink(pathname);
		RA_TRACE("file write failed");
		return -1;
	}
	return 0;
}

char *
ra_file_string_read(const char *pathname)
{
	size_t len;
	char *s;

	if (!(s = ra_file_read(pathname, &len))) {
		RA_TRACE("^");
		return NULL;
	}
	return s;
}

int
ra_file_string_write(const char *pathname, const char *s)
{
	s = s ? s : "";
	if (ra_file_write(pathname, s, strlen(s) + 1)) {
		RA_TRACE("^");
		return -1;
	}
	return 0;
}
