/* Copyright (c) Tony Givargis, 2024-2026 */

#define _GNU_SOURCE

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#if defined(__linux__)
#  include <linux/fs.h>
#endif /* __linux__ */

#if defined(__APPLE__)
#  include <sys/disk.h>
#endif /* __APPLE__ */

#include "ra_device.h"

struct ra_device {
	int fd;
	uint64_t size;
	uint64_t block;
};

ra_device_t
ra_device_open(const char *pathname)
{
	struct ra_device *device;
	struct stat st;
	uint64_t u64;
	uint32_t u32;
	int i, n;

	assert( pathname && strlen(pathname) );

	/* initialize */

	if (!(device = malloc(sizeof (struct ra_device)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(device, 0, sizeof (struct ra_device));
	device->fd = -1;

#if defined(__linux__)
	if (0 > (device->fd = open(pathname, O_RDWR | O_DIRECT))) {
		ra_device_close(device);
		RA_TRACE("unable to open device");
		return NULL;
	}
#endif /* __linux__ */

#if defined(__APPLE__)
	if ((0 > (device->fd = open(pathname, O_RDWR))) ||
	    (0 > fcntl(device->fd, F_NOCACHE, 1))) {
		ra_device_close(device);
		RA_TRACE("unable to open device");
		return NULL;
	}
#endif /* __APPLE__ */

	/* lock */

	n = 5;
	for (i=0; i<n; ++i) {
		if (!flock(device->fd, LOCK_EX | LOCK_NB)) {
			break;
		}
		ra_sleep(2000000);
	}
	if (n <= i) {
		ra_device_close(device);
		RA_TRACE("unable to lock device");
		return NULL;
	}

	/* stats */

	if (fstat(device->fd, &st)) {
		ra_device_close(device);
		RA_TRACE("unable to stat device");
		return NULL;
	}

	/* block? */

	if (!S_ISBLK(st.st_mode)) {
		ra_device_close(device);
		RA_TRACE("not a block device");
		return NULL;
	}

#if defined(__linux__)
	if (ioctl(device->fd, BLKGETSIZE64, &u64) ||
	    ioctl(device->fd, BLKSSZGET, &u32)) {
		ra_device_close(device);
		RA_TRACE("unable to ioctl device");
		return NULL;
	}
	device->size = u64;
	device->block = u32;
#endif /* __linux__ */

#if defined(__APPLE__)
	if (ioctl(device->fd, DKIOCGETBLOCKCOUNT, &u64) ||
	    ioctl(device->fd, DKIOCGETBLOCKSIZE, &u32)) {
		ra_device_close(device);
		RA_TRACE("unable to ioctl device");
		return NULL;
	}
	device->size = u64 * u32;
	device->block = u32;
#endif /* __APPLE__ */

	return device;
}

void
ra_device_close(ra_device_t device)
{
	if (device) {
		if (0 <= device->fd) {
			close(device->fd);
		}
		memset(device, 0, sizeof (struct ra_device));
		RA_FREE(device);
	}
}

int
ra_device_read(ra_device_t device, void *buf, uint64_t off, uint64_t len)
{
	assert( device );
	assert( !len || buf );
	assert( 0 == (off % device->block) );
	assert( 0 == (len % device->block) );

	if (len != (uint64_t)pread(device->fd, buf, (size_t)len, (off_t)off)) {
		RA_TRACE("unable to read device");
		return -1;
	}
	return 0;
}

int
ra_device_write(ra_device_t device,
		const void *buf,
		uint64_t off,
		uint64_t len)
{
	assert( device );
	assert( !len || buf );
	assert( 0 == (off % device->block) );
	assert( 0 == (len % device->block) );

	if (len != (uint64_t)pwrite(device->fd,
				    buf,
				    (size_t)len,
				    (off_t)off)) {
		RA_TRACE("unable to write device");
		return -1;
	}
	return 0;
}

uint64_t
ra_device_size(ra_device_t device)
{
	assert( device );

	return device->size;
}

uint64_t
ra_device_block(ra_device_t device)
{
	assert( device );

	return device->block;
}
