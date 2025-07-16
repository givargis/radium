/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_thread.c
 */

#include <pthread.h>

#include "ra_error.h"
#include "ra_thread.h"

struct ra__thread {
	int good;
	void *ctx;
	pthread_t pthread;
	ra__thread_fnc_t fnc;
};

struct ra__mutex {
	pthread_mutex_t mutex;
};

struct ra__cond {
	ra__mutex_t mutex;
	pthread_cond_t cond;
};

static void *
_thread_(void *ctx)
{
	struct ra__thread *thread;

	assert( ctx );

	thread = (struct ra__thread *)ctx;
	thread->fnc(thread->ctx);
	return NULL;
}

ra__thread_t
ra__thread_open(ra__thread_fnc_t fnc, void *ctx)
{
	struct ra__thread *thread;

	assert( fnc );

	if (!(thread = ra__malloc(sizeof (struct ra__thread)))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	thread->ctx = ctx;
	thread->fnc = fnc;
	if (pthread_create(&thread->pthread, NULL, _thread_, thread)) {
		ra__thread_close(thread);
		RA__ERROR_HALT(RA__ERROR_KERNEL);
		return NULL;
	}
	thread->good = 1;
	return thread;
}

void
ra__thread_close(ra__thread_t thread)
{
	if (thread) {
		if (thread->good) {
			pthread_join(thread->pthread, NULL);
		}
		memset(thread, 0, sizeof (struct ra__thread));
	}
	RA__FREE(thread);
}

int
ra__thread_good(ra__thread_t thread)
{
	return thread && thread->good;
}

ra__mutex_t
ra__mutex_open(void)
{
	struct ra__mutex *mutex;

	if (!(mutex = ra__malloc(sizeof (struct ra__mutex)))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(mutex, 0, sizeof (struct ra__mutex));
	if (pthread_mutex_init(&mutex->mutex, NULL)) {
		ra__mutex_close(mutex);
		RA__ERROR_HALT(RA__ERROR_KERNEL);
		return NULL;
	}
	return mutex;
}

void
ra__mutex_close(ra__mutex_t mutex)
{
	if (mutex) {
		pthread_mutex_destroy(&mutex->mutex);
		memset(mutex, 0, sizeof (struct ra__mutex));
	}
	RA__FREE(mutex);
}

void
ra__mutex_lock(ra__mutex_t mutex)
{
	assert( mutex );

	if (pthread_mutex_lock(&mutex->mutex)) {
		RA__ERROR_HALT(RA__ERROR_KERNEL);
	}
}

void
ra__mutex_unlock(ra__mutex_t mutex)
{
	assert( mutex );

	if (pthread_mutex_unlock(&mutex->mutex)) {
		RA__ERROR_HALT(RA__ERROR_KERNEL);
	}
}

ra__cond_t
ra__cond_open(ra__mutex_t mutex)
{
	struct ra__cond *cond;

	assert( mutex );

	if (!(cond = ra__malloc(sizeof (struct ra__cond)))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(cond, 0, sizeof (struct ra__cond));
	cond->mutex = mutex;
	if (pthread_cond_init(&cond->cond, NULL)) {
		ra__cond_close(cond);
		RA__ERROR_HALT(RA__ERROR_KERNEL);
		return NULL;
	}
	return cond;
}

void
ra__cond_close(ra__cond_t cond)
{
	if (cond) {
		pthread_cond_destroy(&cond->cond);
		memset(cond, 0, sizeof (struct ra__cond));
	}
	RA__FREE(cond);
}

void
ra__cond_signal(ra__cond_t cond)
{
	assert( cond );

	if (pthread_cond_signal(&cond->cond)) {
		RA__ERROR_HALT(RA__ERROR_KERNEL);
	}
}

void
ra__cond_wait(ra__cond_t cond)
{
	assert( cond );

	if (pthread_cond_wait(&cond->cond, &cond->mutex->mutex)) {
		RA__ERROR_HALT(RA__ERROR_KERNEL);
	}
}
