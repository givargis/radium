/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_bitset.h
 */

#ifndef _RA_BITSET_H_
#define _RA_BITSET_H_

#include "../kernel/ra_kernel.h"

typedef struct ra__bitset *ra__bitset_t;

ra__bitset_t ra__bitset_open(uint64_t capacity);

void ra__bitset_close(ra__bitset_t bitset);

uint64_t ra__bitset_reserve(ra__bitset_t bitset, uint64_t n);

uint64_t ra__bitset_release(ra__bitset_t bitset, uint64_t i);

uint64_t ra__bitset_validate(ra__bitset_t bitset, uint64_t i);

uint64_t ra__bitset_utilized(ra__bitset_t bitset);

uint64_t ra__bitset_capacity(ra__bitset_t bitset);

int ra__bitset_bist(void);

#endif // _RA_BITSET_H_
