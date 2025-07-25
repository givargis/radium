/**
 * Copyright (c) Tony Givargis, 2024-2025
 *
 * ra_base64.h
 */

#ifndef _RA_BASE64_H_
#define _RA_BASE64_H_

#include "../root/ra_root.h"

#define RA_BASE64_ENCODE_LEN(n) ( RA_DUP((n), 3) * 4 + 1 )
#define RA_BASE64_DECODE_LEN(n) ( RA_DUP((n), 4) * 3 + 0 )

void ra_base64_encode(const void *buf, size_t len, char *s);

int ra_base64_decode(void *buf, size_t *len, const char *s);

int ra_base64_bist(void);

#endif // _RA_BASE64_H_
