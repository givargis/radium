/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_hash.h
 */

#ifndef _RA_HASH_H_
#define _RA_HASH_H_

#include "../kernel/ra_kernel.h"

uint64_t ra__hash(const void *buf, uint64_t len);

int ra__hash_bist(void);

#endif // _RA_HASH_H_
