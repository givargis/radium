/*
 * Copyright (c) Tony Givargis, 2024-2025
 *
 * ra_thread.c
 */

#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <sched.h>

#include "ra_kernel.h"
#include "ra_thread.h"

struct ra_thread {
	int good;
	void *ctx;
	pthread_t pthread;
	ra_thread_fnc_t fnc;
};

struct ra_mutex {
	pthread_mutex_t mutex;
};

struct ra_cond {
	ra_mutex_t mutex;
	pthread_cond_t cond;
};

static void *
_thread_(void *ctx)
{
	struct ra_thread *thread;

	assert( ctx );

	thread = (struct ra_thread *)ctx;
	thread->fnc(thread->ctx);
	return NULL;
}

ra_thread_t
ra_thread_open(ra_thread_fnc_t fnc, void *ctx)
{
	struct ra_thread *thread;

	assert( fnc );

	if (!(thread = malloc(sizeof (struct ra_thread)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	thread->ctx = ctx;
	thread->fnc = fnc;
	if (pthread_create(&thread->pthread, NULL, _thread_, thread)) {
		ra_thread_close(thread);
		RA_TRACE("kernel");
		return NULL;
	}
	thread->good = 1;
	return thread;
}

void
ra_thread_close(ra_thread_t thread)
{
	if (thread) {
		if (thread->good) {
			pthread_join(thread->pthread, NULL);
		}
		memset(thread, 0, sizeof (struct ra_thread));
	}
	free(thread);
}

int
ra_thread_good(ra_thread_t thread)
{
	return thread && thread->good;
}

ra_mutex_t
ra_mutex_open(void)
{
	struct ra_mutex *mutex;

	if (!(mutex = malloc(sizeof (struct ra_mutex)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(mutex, 0, sizeof (struct ra_mutex));
	if (pthread_mutex_init(&mutex->mutex, NULL)) {
		ra_mutex_close(mutex);
		RA_TRACE("kernel");
		return NULL;
	}
	return mutex;
}

void
ra_mutex_close(ra_mutex_t mutex)
{
	if (mutex) {
		pthread_mutex_destroy(&mutex->mutex);
		memset(mutex, 0, sizeof (struct ra_mutex));
	}
	free(mutex);
}

void
ra_mutex_lock(ra_mutex_t mutex)
{
	assert( mutex );

	if (pthread_mutex_lock(&mutex->mutex)) {
		RA_TRACE("kernel (halting)");
		exit(-1);
	}
}

void
ra_mutex_unlock(ra_mutex_t mutex)
{
	assert( mutex );

	if (pthread_mutex_unlock(&mutex->mutex)) {
		RA_TRACE("kernel (halting)");
		exit(-1);
	}
}

ra_cond_t
ra_cond_open(ra_mutex_t mutex)
{
	struct ra_cond *cond;

	assert( mutex );

	if (!(cond = malloc(sizeof (struct ra_cond)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(cond, 0, sizeof (struct ra_cond));
	cond->mutex = mutex;
	if (pthread_cond_init(&cond->cond, NULL)) {
		ra_cond_close(cond);
		RA_TRACE("kernel");
		return NULL;
	}
	return cond;
}

void
ra_cond_close(ra_cond_t cond)
{
	if (cond) {
		pthread_cond_destroy(&cond->cond);
		memset(cond, 0, sizeof (struct ra_cond));
	}
	free(cond);
}

void
ra_cond_signal(ra_cond_t cond)
{
	assert( cond );

	if (pthread_cond_signal(&cond->cond)) {
		RA_TRACE("kernel (halting)");
		exit(-1);
	}
}

void
ra_cond_wait(ra_cond_t cond)
{
	assert( cond );

	if (pthread_cond_wait(&cond->cond, &cond->mutex->mutex)) {
		RA_TRACE("kernel (halting)");
		exit(-1);
	}
}

void
ra_spinlock_lock(volatile ra_spinlock_t *lock)
{
	assert( lock );

	while (!__sync_bool_compare_and_swap(lock, 0, 1)) {
		sched_yield();
	}
}

void
ra_spinlock_unlock(volatile ra_spinlock_t *lock)
{
	assert( lock );

	while (!__sync_bool_compare_and_swap(lock, 1, 0)) {
		sched_yield();
	}
}
