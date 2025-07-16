/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_spinlock.c
 */

#define _POSIX_C_SOURCE 200809L

#include <sched.h>

#include "ra_core.h"
#include "ra_spinlock.h"

void
ra__spinlock_lock(volatile ra__spinlock_t *lock)
{
	assert( lock );

	while (!__sync_bool_compare_and_swap(lock, 0, 1)) {
		sched_yield();
	}
}

void
ra__spinlock_unlock(volatile ra__spinlock_t *lock)
{
	assert( lock );

	while (!__sync_bool_compare_and_swap(lock, 1, 0)) {
		sched_yield();
	}
}
