/**
 * Copyright (c) Tony Givargis, 2024-2025
 *
 * ra_main.c
 */

#include "root/ra_root.h"
#include "utils/ra_utils.h"

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
	       "\t --nocolor Do not print using color\n"
	       "\n");
}

int
main(int argc, char *argv[])
{
	int notrace = 0;
	int nocolor = 0;
	int bist = 0;

	for (int i=1; i<argc; ++i) {
		if (!strcmp(argv[i], "--help") && (2 == argc)) {
			help();
			return 0;
		}
		else if (!strcmp(argv[i], "--version")) {
			printf("1.0\n");
			return 0;
		}
		else if (!strcmp(argv[i], "--bist")) {
			bist = 1;
		}
		else if (!strcmp(argv[i], "--notrace")) {
			notrace = 1;
		}
		else if (!strcmp(argv[i], "--nocolor")) {
			nocolor = 1;
		}
		else {
			fprintf(stderr, "invalid argument: '%s'\n", argv[i]);
			return -1;
		}
	}
	ra_root_init(notrace, nocolor);
	ra_utils_init();
	return bist ? ra_utils_bist() : stage();
}
