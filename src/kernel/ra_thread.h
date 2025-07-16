/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_thread.h
 */

#ifndef _RA_THREAD_H_
#define _RA_THREAD_H_

typedef struct ra__thread *ra__thread_t;
typedef struct ra__mutex *ra__mutex_t;
typedef struct ra__cond *ra__cond_t;

typedef void (*ra__thread_fnc_t)(void *ctx);

ra__thread_t ra__thread_open(ra__thread_fnc_t fnc, void *ctx);

void ra__thread_close(ra__thread_t thread);

int ra__thread_good(ra__thread_t thread);

ra__mutex_t ra__mutex_open(void);

void ra__mutex_close(ra__mutex_t mutex);

void ra__mutex_lock(ra__mutex_t mutex);

void ra__mutex_unlock(ra__mutex_t mutex);

ra__cond_t ra__cond_open(ra__mutex_t mutex);

void ra__cond_close(ra__cond_t cond);

void ra__cond_signal(ra__cond_t cond);

void ra__cond_wait(ra__cond_t cond);

#endif // _RA_THREAD_H_
