//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_base64.h
//

#ifndef __RA_BASE64_H__
#define __RA_BASE64_H__

#include <stddef.h>

#define RA_BASE64_ENCODE_LEN(n) ( RA_DUP((n), 3) * 4 + 1 )
#define RA_BASE64_DECODE_LEN(n) ( RA_DUP((n), 4) * 3 + 0 )

void ra_base64_encode(const void *buf, size_t len, char *s);

int ra_base64_decode(void *buf, size_t *len, const char *s);

int ra_base64_test(void);

#endif // __RA_BASE64_H__
