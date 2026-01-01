/* Copyright (c) Tony Givargis, 2024-2026 */

#ifndef __RA_DEVICE_H__
#define __RA_DEVICE_H__

#include "ra_kernel.h"

typedef struct ra_device *ra_device_t;

ra_device_t ra_device_open(const char *pathname);

void ra_device_close(ra_device_t device);

int ra_device_read(ra_device_t device, void *buf, uint64_t off, uint64_t len);

int ra_device_write(ra_device_t device,
		    const void *buf,
		    uint64_t off,
		    uint64_t len);

uint64_t ra_device_size(ra_device_t device);

uint64_t ra_device_block(ra_device_t device);

#endif /* __RA_DEVICE_H__ */
