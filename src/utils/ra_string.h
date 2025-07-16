/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_string.h
 */

#ifndef _RA_STRING_H_
#define _RA_STRING_H_

typedef struct ra__string *ra__string_t;

ra__string_t ra__string_open(void);

void ra__string_close(ra__string_t string, const char **buf);

void ra__string_truncate(ra__string_t string);

int ra__string_append(ra__string_t string, const char *format, ...);

const char *ra__string_buf(ra__string_t string);

int ra__string_bist(void);

#endif // _RA_STRING_H_
