/* Copyright (c) Tony Givargis, 2024-2026 */

#include <pthread.h>
#include <sched.h>

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
	thread->good = 1;
	if (pthread_create(&thread->pthread, NULL, _thread_, thread)) {
		thread->good = 0;
		ra_thread_close(thread);
		RA_TRACE("system failure detected");
		return NULL;
	}
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
		RA_FREE(thread);
	}
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
		RA_TRACE("system failure detected");
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
		RA_FREE(mutex);
	}
}

void
ra_mutex_lock(ra_mutex_t mutex)
{
	assert( mutex );

	if (pthread_mutex_lock(&mutex->mutex)) {
		RA_TRACE("system failure detected (abort)");
		abort();
	}
}

void
ra_mutex_unlock(ra_mutex_t mutex)
{
	assert( mutex );

	if (pthread_mutex_unlock(&mutex->mutex)) {
		RA_TRACE("system failure detected (abort)");
		abort();
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
		RA_TRACE("system failure detected");
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
		RA_FREE(cond);
	}
}

void
ra_cond_signal(ra_cond_t cond)
{
	assert( cond );

	if (pthread_cond_signal(&cond->cond)) {
		RA_TRACE("system failure detected (abort)");
		abort();
	}
}

void
ra_cond_wait(ra_cond_t cond)
{
	assert( cond );

	if (pthread_cond_wait(&cond->cond, &cond->mutex->mutex)) {
		RA_TRACE("system failure detected (abort)");
		abort();
	}
}
