/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_file.h
 */

#ifndef _RA_FILE_H_
#define _RA_FILE_H_

#include "ra_core.h"

#define RA__FILE_MODE_RD       0x00
#define RA__FILE_MODE_RDWR     0x01
#define RA__FILE_MODE_CREATE   0x02
#define RA__FILE_MODE_TRUNCATE 0x04

typedef int (*ra__file_fnc_t)(void *ctx, const char *pathname);

typedef struct ra__file *ra__file_t;

int ra__file_dir(const char *pathname, ra__file_fnc_t fnc, void *ctx);

int ra__file_unlink(const char *pathname);

ra__file_t ra__file_open(const char *pathname, int mode);

void ra__file_close(ra__file_t file);

int ra__file_size(ra__file_t file, uint64_t *size);

int ra__file_read(ra__file_t file, void *buf, uint64_t off, uint64_t len);

int ra__file_write(ra__file_t file,
		   const void *buf,
		   uint64_t off,
		   uint64_t len);

const char *ra__file_tmpfile(const char *ext);

const char *ra__file_read_content(const char *pathname);

int ra__file_write_content(const char *pathname, const char *content);

#endif // _RA_FILE_H_
