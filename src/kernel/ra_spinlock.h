/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_spinlock.h
 */

#ifndef _RA_SPINLOCK_H_
#define _RA_SPINLOCK_H_

typedef int ra__spinlock_t;

void ra__spinlock_lock(volatile ra__spinlock_t *lock);

void ra__spinlock_unlock(volatile ra__spinlock_t *lock);

#endif // _RA_SPINLOCK_H_
