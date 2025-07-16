/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_file.c
 */

#define _POSIX_C_SOURCE 200809L

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include "ra_error.h"
#include "ra_file.h"

struct ra__file {
	int fd;
};

static int
_dir_(const char *pathname, ra__file_fnc_t fnc, void *ctx)
{
	struct dirent *dirent;
	struct stat stat_;
	char *pathname_;
	uint64_t n;
	DIR *dir;

	if (stat(pathname, &stat_) ||
	    (!S_ISDIR(stat_.st_mode) && !S_ISREG(stat_.st_mode))) {
		return 0;
	}
	if (S_ISREG(stat_.st_mode)) {
		if (fnc(ctx, pathname)) {
			RA__ERROR_TRACE(0);
			return -1;
		}
		return 0;
	}
	if (!(dir = opendir(pathname))) {
		RA__ERROR_TRACE(RA__ERROR_KERNEL);
		return -1;
	}
	while ((dirent = readdir(dir))) {
		if (!strcmp(dirent->d_name, ".") ||
		    !strcmp(dirent->d_name, "..")) {
			continue;
		}
		n = ra__strlen(pathname) + ra__strlen(dirent->d_name) + 2;
		if (!(pathname_ = ra__malloc(n))) {
			closedir(dir);
			RA__ERROR_TRACE(0);
			return -1;
		}
		snprintf(pathname_, n, "%s/%s", pathname, dirent->d_name);
		if (_dir_(pathname_, fnc, ctx)) {
			closedir(dir);
			RA__FREE(pathname_);
			RA__ERROR_TRACE(0);
			return -1;
		}
		RA__FREE(pathname_);
	}
	closedir(dir);
	return 0;
}

int
ra__file_dir(const char *pathname, ra__file_fnc_t fnc, void *ctx)
{
	assert( ra__strlen(pathname) );
	assert( fnc );

	if (_dir_(pathname, fnc, ctx)) {
		RA__ERROR_TRACE(0);
		return -1;
	}
	return 0;
}

int
ra__file_unlink(const char *pathname)
{
	assert( ra__strlen(pathname) );

	errno = 0;
	if (remove(pathname)) {
		if (ENOENT != errno) {
			RA__ERROR_TRACE(RA__ERROR_FILE_UNLINK);
			return -1;
		}
	}
	return 0;
}

ra__file_t
ra__file_open(const char *pathname, int mode_)
{
	struct ra__file *file;
	int mode;

	assert( ra__strlen(pathname) );

	if (!(file = ra__malloc(sizeof (struct ra__file)))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(file, 0, sizeof (struct ra__file));
	mode  = 0;
	mode |= (RA__FILE_MODE_RD & mode_) ? O_RDONLY : 0;
	mode |= (RA__FILE_MODE_RDWR & mode_) ? O_RDWR : 0;
	mode |= (RA__FILE_MODE_CREATE & mode_) ? O_CREAT : 0;
	mode |= (RA__FILE_MODE_TRUNCATE & mode_) ? O_TRUNC : 0;
	if (RA__FILE_MODE_CREATE & mode) {
		file->fd = open(pathname, mode, 0644);
	}
	else {
		file->fd = open(pathname, mode);
	}
	if (0 >= file->fd) {
		ra__file_close(file);
		RA__ERROR_TRACE(RA__ERROR_FILE_OPEN);
		return NULL;
	}
	return file;
}

void
ra__file_close(ra__file_t file)
{
	if (file) {
		if (0 < file->fd) {
			close(file->fd);
		}
		memset(file, 0, sizeof (struct ra__file));
	}
	RA__FREE(file);
}

int
ra__file_size(ra__file_t file, uint64_t *size)
{
	struct stat st;

	assert( file );
	assert( size );

	if (fstat(file->fd, &st) || !S_ISREG(st.st_mode)) {
		RA__ERROR_TRACE(RA__ERROR_FILE_STAT);
		return -1;
	}
	(*size) = (uint64_t)st.st_size;
	return 0;
}

int
ra__file_read(ra__file_t file, void *buf_, uint64_t off, uint64_t len)
{
	char *buf = (char *)buf_;
	ssize_t n;

	assert( file );
	assert( !len || buf );

	while (len) {
		if (0 >= (n = pread(file->fd, buf, (size_t)len, (off_t)off))) {
			RA__ERROR_TRACE(RA__ERROR_FILE_READ);
			return -1;
		}
		off += (uint64_t)n;
		len -= (uint64_t)n;
	}
	return 0;
}

int
ra__file_write(ra__file_t file, const void *buf_, uint64_t off, uint64_t len)
{
	const char *buf = (const char *)buf_;
	ssize_t n;

	assert( file );
	assert( !len || buf );

	while (len) {
		if (0 > (n = pwrite(file->fd, buf, (size_t)len, (off_t)off))) {
			RA__ERROR_TRACE(RA__ERROR_FILE_WRITE);
			return -1;
		}
		off += (uint64_t)n;
		len -= (uint64_t)n;
	}
	return 0;
}

const char *
ra__file_tmpfile(const char *ext)
{
	const char *dir;
	uint64_t len;
	char *buf;

	dir = getenv("TMPDIR");
	len = ra__strlen(dir) + ra__strlen(ext) + 32;
	if (!(buf = ra__malloc(len))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	snprintf(buf,
		 len,
		 "%s%s_%x%08x%s",
		 ra__strlen(dir) ? dir : "",
		 ra__strlen(dir) ? "/" : "",
		 (unsigned)(ra__time() % 16),
		 (unsigned)rand(),
		 ra__strlen(ext) ? ext : "");
	return buf;
}

const char *
ra__file_read_content(const char *pathname)
{
	const uint64_t PAD = 8;
	ra__file_t file;
	uint64_t size;
	char *content;

	assert( ra__strlen(pathname) );

	content = NULL;
	if (!(file = ra__file_open(pathname, RA__FILE_MODE_RD)) ||
	    ra__file_size(file, &size) ||
	    !(content = ra__malloc(size + PAD)) ||
	    ra__file_read(file, content, 0, size)) {
		ra__file_close(file);
		RA__FREE(content);
		RA__ERROR_TRACE(0);
		return NULL;
	}
	ra__file_close(file);
	memset(content + size, 0, PAD);
	return content;
}

int
ra__file_write_content(const char *pathname, const char *content)
{
	ra__file_t file;

	assert( ra__strlen(pathname) );
	assert( content );

	if (!(file = ra__file_open(pathname,
				   RA__FILE_MODE_RDWR |
				   RA__FILE_MODE_CREATE |
				   RA__FILE_MODE_TRUNCATE))) {
		RA__ERROR_TRACE(0);
		return -1;
	}
	if (ra__file_write(file, content, 0, ra__strlen(content))) {
		ra__file_close(file);
		ra__file_unlink(pathname);
		RA__ERROR_TRACE(0);
		return -1;
	}
	ra__file_close(file);
	return 0;
}
