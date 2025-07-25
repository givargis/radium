/**
 * Copyright (c) Tony Givargis, 2024-2025
 *
 * ra_hash.h
 */

#ifndef _RA_HASH_H_
#define _RA_HASH_H_

#include "../root/ra_root.h"

uint64_t ra_hash(const void *buf, size_t len);

int ra_hash_bist(void);

#endif // _RA_HASH_H_
