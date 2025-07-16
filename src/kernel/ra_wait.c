/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_wait.c
 */

#include <unistd.h>
#include <signal.h>

#include "ra_error.h"
#include "ra_wait.h"

static int _sigint_;

static void
_signal_(int signum)
{
	if (SIGINT == signum) {
		__sync_fetch_and_add(&_sigint_, 1);
		printf("\r  \r");
	}
}

void
ra__wait(void)
{
	_sigint_ = 0;
	if ((SIG_ERR == signal(SIGHUP, _signal_)) ||
	    (SIG_ERR == signal(SIGPIPE, _signal_)) ||
	    (SIG_ERR == signal(SIGINT, _signal_))) {
		RA__ERROR_HALT(RA__ERROR_KERNEL);
	}
	while (!__sync_fetch_and_add(&_sigint_, 0)) {
		sleep(1);
	}
	if ((SIG_ERR == signal(SIGHUP, SIG_DFL)) ||
	    (SIG_ERR == signal(SIGPIPE, SIG_DFL)) ||
	    (SIG_ERR == signal(SIGINT, SIG_DFL))) {
		RA__ERROR_HALT(RA__ERROR_KERNEL);
	}
}
