/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_jitc.c
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <spawn.h>
#include <dlfcn.h>

#include "ra_error.h"
#include "ra_jitc.h"

struct ra__jitc {
	void *handle;
};

int
ra__jitc_compile(const char *input, const char *output)
{
	extern char **environ;
	const char *argv[] = {
		"/usr/bin/gcc",
		"-std=c99",
		"-pedantic",
		"-Wall",
		"-Wextra",
		"-Werror",
		"-Wfatal-errors",
		"-fPIC",
		"-O3",
		"-shared",
		"-o",
		"<output>",
		"<input>",
		NULL
	};
	pid_t pid, pid_;
	int status;

	assert( ra__strlen(input) );
	assert( ra__strlen(output) );

	argv[RA__ARRAY_SIZE(argv) - 2] = input;
	argv[RA__ARRAY_SIZE(argv) - 3] = output;
	if (posix_spawn(&pid,
			argv[0],
			NULL,
			NULL,
			(char * const *)argv,
			environ)) {
		RA__ERROR_TRACE(RA__ERROR_KERNEL);
		return -1;
	}
	for (;;) {
		errno = 0;
		pid_ = waitpid(pid, &status, 0);
		if ((-1 == pid_) && (EINTR == errno)) {
			continue;
		}
		if ((-1 == pid_) || (pid_ != pid) || !WIFEXITED(status)) {
			RA__ERROR_HALT(RA__ERROR_SOFTWARE);
			return -1;
		}
		if (WEXITSTATUS(status)) {
			RA__ERROR_TRACE(RA__ERROR_KERNEL);
			return -1;
		}
		break;
	}
	return 0;
}

ra__jitc_t
ra__jitc_open(const char *pathname)
{
	struct ra__jitc *jitc;

	assert( ra__strlen(pathname) );

	if (!(jitc = ra__malloc(sizeof (struct ra__jitc)))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(jitc, 0, sizeof (struct ra__jitc));
	if (!(jitc->handle = dlopen(pathname, RTLD_LAZY | RTLD_LOCAL))) {
		ra__jitc_close(jitc);
		RA__ERROR_TRACE(RA__ERROR_KERNEL);
		return NULL;
	}
	return jitc;
}

void
ra__jitc_close(ra__jitc_t jitc)
{
	int errno_;

	if (jitc) {
		if (jitc->handle) {
			errno_ = errno;
			dlclose(jitc->handle);
			errno = errno_;
		}
		memset(jitc, 0, sizeof (struct ra__jitc));
	}
	RA__FREE(jitc);
}

long
ra__jitc_lookup(ra__jitc_t jitc, const char *symbol)
{
	assert( jitc );
	assert( jitc->handle );
	assert( ra__strlen(symbol) );

	return (long)dlsym(jitc->handle, symbol);
}
