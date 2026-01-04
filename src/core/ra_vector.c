/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_vector.h"

struct ra_vector {
	void **memory;
	uint64_t size;
	uint64_t items;
};

ra_vector_t
ra_vector_open(void)
{
	struct ra_vector *vector;

	if (!(vector = malloc(sizeof (struct ra_vector)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(vector, 0, sizeof (struct ra_vector));
	return vector;
}

void
ra_vector_close(ra_vector_t vector)
{
	uint64_t i;

	if (vector) {
		if (vector->memory) {
			for (i=0; i<vector->items; ++i) {
				RA_FREE(vector->memory[i]);
			}
		}
		RA_FREE(vector->memory);
		RA_FREE(vector);
	}
}

void *
ra_vector_append(ra_vector_t vector, size_t n)
{
	uint64_t size;
	void **memory;
	size_t m;

	assert( vector && (0 < n) );

	if (vector->items >= vector->size) {
		size = vector->size + 100;
		m = size * sizeof (vector->memory[0]);
		if (!(memory = realloc(vector->memory, m))) {
			RA_TRACE("out of memory");
			return NULL;
		}
		memset(&memory[vector->size],
		       0,
		       (size - vector->size) * sizeof (vector->memory[0]));
		vector->size = size;
		vector->memory = memory;
	}
	if (!(vector->memory[vector->items] = malloc(n))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(vector->memory[vector->items], 0, n);
	return vector->memory[vector->items++];
}

void *
ra_vector_lookup(ra_vector_t vector, uint64_t i)
{
	assert( vector && (vector->items > i) );

	return (void *)vector->memory[i];
}

uint64_t
ra_vector_items(ra_vector_t vector)
{
	assert( vector );

	return vector->items;
}
