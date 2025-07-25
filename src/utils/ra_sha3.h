/**
 * Copyright (c) Tony Givargis, 2024-2025
 *
 * ra_sha3.h
 */

#ifndef _RA_SHA3_H_
#define _RA_SHA3_H_

#include "../root/ra_root.h"

#define RA_SHA3_LEN 32

void ra_sha3(const void *buf, size_t len, void *out);

int ra_sha3_bist(void);

#endif // _RA_SHA3_H_
