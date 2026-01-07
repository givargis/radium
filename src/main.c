/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_lexer.h"

int
main(int argc, char *argv[])
{
	ra_lexer_t lexer;
	const char *s;
	ra_csv_t csv;

	ra_core_init();
	if (2 != argc) {
		fprintf(stderr, "usage: radium pathname\n");
		return -1;
	}
	if (!(lexer = ra_lexer_open(argv[1]))) {
		RA_TRACE("^");
		return -1;
	}
	if (!(s = ra_lexer_csv(lexer))) {
		ra_lexer_close(lexer);
		RA_TRACE("^");
		return -1;
	}
	if (!(csv = ra_csv_open(s)) || ra_csv_print(csv)) {
		ra_lexer_close(lexer);
		ra_csv_close(csv);
		RA_FREE(s);
		RA_TRACE("^");
		return -1;
	}
	ra_lexer_close(lexer);
	ra_csv_close(csv);
	RA_FREE(s);
	return 0;
}
