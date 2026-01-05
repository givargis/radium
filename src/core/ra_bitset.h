/* Copyright (c) Tony Givargis, 2024-2026 */

#ifndef __RA_BITSET_H__
#define __RA_BITSET_H__

#include "ra_kernel.h"

typedef struct ra_bitset *ra_bitset_t;

ra_bitset_t ra_bitset_open(uint64_t size);

void ra_bitset_close(ra_bitset_t bitset);

uint64_t ra_bitset_reserve(ra_bitset_t bitset, uint64_t n);

uint64_t ra_bitset_release(ra_bitset_t bitset, uint64_t i);

uint64_t ra_bitset_validate(ra_bitset_t bitset, uint64_t i);

uint64_t ra_bitset_utilized(ra_bitset_t bitset);

uint64_t ra_bitset_size(ra_bitset_t bitset);

int ra_bitset_test(void);

#endif /* __RA_BITSET_H__ */
