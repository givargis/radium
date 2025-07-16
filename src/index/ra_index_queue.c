/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_index_queue.c
 */

#include "ra_index_queue.h"

struct ra__index_queue {
	void **vals;
	uint64_t size;
	uint64_t head;
	uint64_t tail;
	uint64_t capacity;
};

ra__index_queue_t
ra__index_queue_open(uint64_t capacity)
{
	struct ra__index_queue *queue;
	uint64_t n;

	assert( capacity );

	if (!(queue = ra__malloc(sizeof (struct ra__index_queue)))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(queue, 0, sizeof (struct ra__index_queue));
	queue->capacity = capacity;
	n = queue->capacity * sizeof (queue->vals[0]);
	if (!(queue->vals = ra__malloc(n))) {
		ra__index_queue_close(queue);
		RA__ERROR_TRACE(0);
		return NULL;
	}
	return queue;
}

void
ra__index_queue_close(ra__index_queue_t queue)
{
	if (queue) {
		RA__FREE(queue->vals);
		memset(queue, 0, sizeof (struct ra__index_queue));
	}
	RA__FREE(queue);
}

void
ra__index_queue_push(ra__index_queue_t queue, void *v)
{
	assert( queue );
	assert( queue->size < queue->capacity );

	queue->vals[queue->head] = v;
	queue->head = (queue->head + 1) % queue->capacity;
	++queue->size;
}

void *
ra__index_queue_pop(ra__index_queue_t queue)
{
	void *v;

	assert( queue );
	assert( queue->size );

	v = queue->vals[queue->tail];
	queue->tail = (queue->tail + 1) % queue->capacity;
	--queue->size;
	return v;
}

int
ra__index_queue_empty(ra__index_queue_t queue)
{
	assert( queue );

	return queue->size ? 0 : 1;
}
