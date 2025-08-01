//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_sha3.h
//

#ifndef __RA_SHA3_H__
#define __RA_SHA3_H__

#include <stddef.h>

#define RA_SHA3_LEN 32

void ra_sha3(const void *buf, size_t len, void *out);

int ra_sha3_test(void);

#endif // __RA_SHA3_H__
