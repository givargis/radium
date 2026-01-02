/* Copyright (c) Tony Givargis, 2024-2026 */

#ifndef __RA_BASE64_H__
#define __RA_BASE64_H__

#include "ra_kernel.h"

#define RA_BASE64_ENCODE_LEN(n) ( ((((n) + 2) / 3) * 4) + 1 )
#define RA_BASE64_DECODE_LEN(n) ( ((((n) + 3) / 4) * 3) + 0 )

int ra_base64_encode(const void *buf, size_t len, char *s);

int ra_base64_decode(void *buf, size_t *len, const char *s);

#endif /* __RA_BASE64_H__ */
