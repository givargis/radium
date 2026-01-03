/* Copyright (c) Tony Givargis, 2024-2026 */

#ifndef __RA_VECTOR_H__
#define __RA_VECTOR_H__

#include "ra_kernel.h"

typedef struct ra_vector *ra_vector_t;

ra_vector_t ra_vector_open(void);

void ra_vector_close(ra_vector_t vector);

void *ra_vector_append(ra_vector_t vector, size_t n);

void *ra_vector_lookup(ra_vector_t vector, uint64_t i);

uint64_t ra_vector_items(ra_vector_t vector);

#endif /* __RA_VECTOR_H__ */
