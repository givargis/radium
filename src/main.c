/* Copyright (c) Tony Givargis, 2024-2026 */

#include "core/ra_core.h"

#include "ra_lexer.h"

int
main(void)
{
	ra_lexer_t lexer;

	ra_core_init();

	lexer = ra_lexer_open("1.+int->\"blah\"");

	ra_lexer_close(lexer);
	return 0;
}
