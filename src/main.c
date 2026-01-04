/* Copyright (c) Tony Givargis, 2024-2026 */

#include "core/ra_core.h"

#include "ra_lexer.h"

int
main(int argc, char *argv[])
{
	ra_lexer_t lexer;
	const char *s;
	uint64_t i;

	(void)argc;
	(void)argv;

	ra_core_init();
	ra_bigint_test();

	if (!(s = ra_file_string_read("test"))) {
		return -1;
	}
	if (!(lexer = ra_lexer_open(s))) {
		RA_FREE(s);
		return -1;
	}
	for (i=0; i<ra_lexer_items(lexer); ++i) {
		const struct ra_lexer_token *token;
		token = ra_lexer_lookup(lexer, i);
		printf("%2d %2u %2u %s\n",
		       token->op,
		       token->lineno,
		       token->column,
		       token->s);
	}
	RA_FREE(s);
	ra_lexer_close(lexer);
	return 0;
}
