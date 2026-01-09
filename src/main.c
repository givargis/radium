/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_parser.h"

int
main(int argc, char *argv[])
{
	ra_parser_t parser;

	ra_trace_enabled = 1;
	ra_core_init();
	if (2 != argc) {
		fprintf(stderr, "usage: radium pathname\n");
		return -1;
	}
	if (!(parser = ra_parser_open(argv[1]))) {
		RA_TRACE("^");
		return -1;
	}
	if (ra_parser_print(parser)) {
		ra_parser_close(parser);
		RA_TRACE("^");
		return -1;
	}
	ra_parser_close(parser);
	return 0;
}
