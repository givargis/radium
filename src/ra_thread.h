//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_thread.h
//

#ifndef __RA_THREAD_H__
#define __RA_THREAD_H__

typedef struct ra_thread *ra_thread_t;
typedef struct ra_mutex *ra_mutex_t;
typedef struct ra_cond *ra_cond_t;
typedef int ra_spinlock_t;

typedef void (*ra_thread_fnc_t)(void *ctx);

ra_thread_t ra_thread_open(ra_thread_fnc_t fnc, void *ctx);

void ra_thread_close(ra_thread_t thread);

int ra_thread_good(ra_thread_t thread);

ra_mutex_t ra_mutex_open(void);

void ra_mutex_close(ra_mutex_t mutex);

void ra_mutex_lock(ra_mutex_t mutex);

void ra_mutex_unlock(ra_mutex_t mutex);

ra_cond_t ra_cond_open(ra_mutex_t mutex);

void ra_cond_close(ra_cond_t cond);

void ra_cond_signal(ra_cond_t cond);

void ra_cond_wait(ra_cond_t cond);

void ra_spinlock_lock(volatile ra_spinlock_t *lock);

void ra_spinlock_unlock(volatile ra_spinlock_t *lock);

#endif // __RA_THREAD_H__
