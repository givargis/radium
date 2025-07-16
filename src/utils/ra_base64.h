/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_base64.h
 */

#ifndef _RA_BASE64_H_
#define _RA_BASE64_H_

#include "../kernel/ra_kernel.h"

#define RA__BASE64_ENCODE_LEN(n) ( RA__DUP((n), 3) * 4 )
#define RA__BASE64_DECODE_LEN(n) ( RA__DUP((n), 4) * 3 )

void ra__base64_encode(const void *buf, uint64_t len, char *s);

int ra__base64_decode(void *buf, uint64_t *len, const char *s);

int ra__base64_bist(void);

#endif // _RA_BASE64_H_
