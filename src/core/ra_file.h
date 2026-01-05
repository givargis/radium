/* Copyright (c) Tony Givargis, 2024-2026 */

#ifndef __RA_FILE_H__
#define __RA_FILE_H__

#include "ra_kernel.h"

void *ra_file_read(const char *pathname, size_t *len);

int ra_file_write(const char *pathname, const void *buf, size_t len);

char *ra_file_string_read(const char *pathname);

int ra_file_string_write(const char *pathname, const char *s);

int ra_file_test(void);

#endif /* __RA_FILE_H__ */
