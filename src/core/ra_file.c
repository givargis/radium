/* Copyright (c) Tony Givargis, 2024-2026 */

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "ra_file.h"

static int
_dir_(const char *pathname, ra_file_fnc_t fnc, void *ctx)
{
	struct dirent *dirent;
	struct stat stat_;
	char *pathname_;
	DIR *dir;
	size_t n;

	if (lstat(pathname, &stat_)) {
		return 0;
	}
	if (S_ISREG(stat_.st_mode)) {
		if (fnc(ctx, pathname)) {
			RA_TRACE("^");
			return -1;
		}
		return 0;
	}
	if (!S_ISDIR(stat_.st_mode)) {
		return 0;
	}
	if (!(dir = opendir(pathname))) {
		RA_TRACE("unable to open directory");
		return -1;
	}
	while ((dirent = readdir(dir))) {
		if (!strcmp(dirent->d_name, ".") ||
		    !strcmp(dirent->d_name, "..")) {
			continue;
		}
		n = strlen(pathname) + strlen(dirent->d_name) + 2;
		if (!(pathname_ = malloc(n))) {
			closedir(dir);
			RA_TRACE("out of memory");
			return -1;
		}
		ra_sprintf(pathname_, n, "%s/%s", pathname, dirent->d_name);
		if (_dir_(pathname_, fnc, ctx)) {
			closedir(dir);
			RA_FREE(pathname_);
			RA_TRACE("^");
			return -1;
		}
		RA_FREE(pathname_);
	}
	closedir(dir);
	return 0;
}

int
ra_file_dir(const char *pathname, ra_file_fnc_t fnc, void *ctx)
{
	assert( pathname && (*pathname) && fnc );

	if (_dir_(pathname, fnc, ctx)) {
		RA_TRACE("^");
		return -1;
	}
	return 0;
}

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
	if (ra_file_write(pathname, s, strlen(s))) {
		RA_TRACE("^");
		return -1;
	}
	return 0;
}

int
ra_file_test(void)
{
	const char *pathname, *s;

	/* zero-byte write/read */

	if (!(pathname = ra_pathname(NULL)) ||
	    ra_file_string_write(pathname, "")) {
		RA_FREE(pathname);
		RA_TRACE("^");
		return -1;
	}
	if (!(s = ra_file_string_read(pathname)) || strlen(s)) {
		ra_unlink(pathname);
		RA_FREE(pathname);
		RA_FREE(s);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	RA_FREE(s);

	/* single-byte write/read */

	if (ra_file_string_write(pathname, "x")) {
		RA_FREE(pathname);
		RA_TRACE("^");
		return -1;
	}
	if (!(s = ra_file_string_read(pathname)) ||
	    (1 != strlen(s)) ||
	    ('x' != (*s))) {
		ra_unlink(pathname);
		RA_FREE(pathname);
		RA_FREE(s);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	RA_FREE(s);

	/* multi-byte write */

	if (ra_file_string_write(pathname, pathname)) {
		RA_FREE(pathname);
		RA_TRACE("^");
		return -1;
	}
	if (!(s = ra_file_string_read(pathname)) ||
	    (strlen(pathname) != strlen(s)) ||
	    strcmp(pathname, s)) {
		ra_unlink(pathname);
		RA_FREE(pathname);
		RA_FREE(s);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	ra_unlink(pathname);
	RA_FREE(pathname);
	RA_FREE(s);
	return 0;
}
