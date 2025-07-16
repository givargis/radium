/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_sha3.h
 */

#ifndef _RA_SHA3_H_
#define _RA_SHA3_H_

#include "../kernel/ra_kernel.h"

#define RA__SHA3_LEN 32

void ra__sha3(const void *buf, uint64_t len, void *out);

int ra__sha3_bist(void);

#endif // _RA_SHA3_H_
