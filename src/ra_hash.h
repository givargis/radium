//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_hash.h
//

#ifndef __RA_HASH_H__
#define __RA_HASH_H__

#include <stdint.h>
#include <stddef.h>

uint64_t ra_hash(const void *buf, size_t len);

int ra_hash_test(void);

#endif // __RA_HASH_H__
