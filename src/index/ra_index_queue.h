/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_index_queue.h
 */

#ifndef _RA_INDEX_QUEUE_H_
#define _RA_INDEX_QUEUE_H_

#include "../utils/ra_utils.h"

typedef struct ra__index_queue *ra__index_queue_t;

ra__index_queue_t ra__index_queue_open(uint64_t capacity);

void ra__index_queue_close(ra__index_queue_t queue);

void ra__index_queue_push(ra__index_queue_t queue, void *v);

void *ra__index_queue_pop(ra__index_queue_t queue);

int ra__index_queue_empty(ra__index_queue_t queue);

#endif // _RA_INDEX_QUEUE_H_
