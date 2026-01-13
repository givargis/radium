/* Copyright (c) Tony Givargis, 2024-2026 */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dlfcn.h>

#include "ra_file.h"
#include "ra_jitc.h"

struct ra_jitc {
	void *handle;
};

int
ra_jitc_compile(const char *input, const char *output)
{
	const char *argv[] = {
		"/usr/bin/gcc",
		"-O3",
		"-fpic",
		"-shared",
		"-o",
		NULL, /* output */
		NULL, /* input */
		NULL
	};
	pid_t pid, pid_;
	int status;

	assert( input && (*input) );
	assert( output && (*output) );

	argv[RA_ARRAY_SIZE(argv) - 2] = input;
	argv[RA_ARRAY_SIZE(argv) - 3] = output;
	if (0 > (pid = fork())) {
		RA_TRACE("unable to fork child process");
		return -1;
	}
	else if (!pid) {
		execv(argv[0], (char * const *)argv);
		RA_TRACE("unable to execute child process (abort)");
		abort();
		return -1;
	}
	else {
		for (;;) {
			pid_ = waitpid(pid, &status, 0);
			if ((-1 == pid_) && (EINTR == errno)) {
				continue;
			}
			break;
		}
		if ((-1 == pid_) || (pid_ != pid) || !WIFEXITED(status)) {
			RA_TRACE("system failure detected (abort)");
			abort();
			return -1;
		}
		if (WEXITSTATUS(status)) {
			RA_TRACE("system failure detected");
			return -1;
		}
	}
	return 0;
}

ra_jitc_t
ra_jitc_open(const char *pathname)
{
	struct ra_jitc *jitc;

	assert( pathname && (*pathname) );

	if (!(jitc = malloc(sizeof (struct ra_jitc)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(jitc, 0, sizeof (struct ra_jitc));
	if (!(jitc->handle = dlopen(pathname, RTLD_LAZY | RTLD_LOCAL))) {
		ra_jitc_close(jitc);
		RA_TRACE("unable to load library");
		return NULL;
	}
	return jitc;
}

void
ra_jitc_close(ra_jitc_t jitc)
{
	if (jitc) {
		if (jitc->handle) {
			dlclose(jitc->handle);
		}
		memset(jitc, 0, sizeof (struct ra_jitc));
		RA_FREE(jitc);
	}
}

long
ra_jitc_lookup(ra_jitc_t jitc, const char *symbol)
{
	assert( jitc );
	assert( jitc->handle );
	assert( symbol && (*symbol) );

	return (long)dlsym(jitc->handle, symbol);
}

int
ra_jitc_test(void)
{
	const char * const PRG = "int fnc(int a) { return a + 1; }\n";
	const char *input, *output;
	int (*fnc)(int);
	ra_jitc_t jitc;

	input = NULL;
	output = NULL;
	if (!(input = ra_pathname(".c")) ||
	    !(output = ra_pathname(".so")) ||
	    ra_file_string_write(input, PRG) ||
	    ra_jitc_compile(input, output) ||
	    !(jitc = ra_jitc_open(output))) {
		ra_unlink(input);
		ra_unlink(output);
		RA_FREE(input);
		RA_FREE(output);
		RA_TRACE("^");
		return -1;
	}
	ra_unlink(input);
	ra_unlink(output);
	RA_FREE(input);
	RA_FREE(output);
	if (!(fnc = (int (*)(int))ra_jitc_lookup(jitc, "fnc")) ||
	    (17 != fnc(16))) {
		ra_jitc_close(jitc);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	ra_jitc_close(jitc);
	return 0;
}
