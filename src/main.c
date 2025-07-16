/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * main.c
 */

#include "index/ra_index.h"
#include "lang/ra_lang.h"

#define VERSION 10

static int
stage(void)
{
	return 0;
}

static void
help(void)
{
	printf("\n"
	       "Usage: radium [options]\n"
	       "\n"
	       "Options:\n"
	       "\t --help    Print the help menu and exit\n"
	       "\t --version Print the version string and exit\n"
	       "\t --bist    Run the built-in-test mode and exit\n"
	       "\t --notrace Do not print error traces\n"
	       "\t --nocolor Do not use terminal colors\n"
	       "\n");
}

int
main(int argc, char *argv[])
{
	int notrace = 0;
	int nocolor = 0;
	int bist = 0;
	int i;

	for (i=1; i<argc; ++i) {
		if (!strcmp(argv[i], "--help") && (2 == argc)) {
			help();
			return 0;
		}
		else if (!strcmp(argv[i], "--version")) {
			printf("%d.%d\n", VERSION / 10, VERSION % 10);
			return 0;
		}
		else if (!strcmp(argv[i], "--notrace")) {
			notrace = 1;
		}
		else if (!strcmp(argv[i], "--nocolor")) {
			nocolor = 1;
		}
		else if (!strcmp(argv[i], "--bist")) {
			bist = 1;
		}
		else {
			fprintf(stderr, "bad argument: '%s'\n", argv[i]);
			return -1;
		}
	}
	ra__kernel_init(notrace, nocolor);
	ra__utils_init();
	if (bist) {
		if (ra__utils_bist() || ra__index_bist()) {
			RA__ERROR_TRACE(0);
			return -1;
		}
		return 0;
	}
	return stage();
}
