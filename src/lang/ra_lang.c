/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_parser.h"
#include "ra_evaluate.h"
#include "ra_lang.h"

struct ra_lang {
	ra_parser_t parser;
};

ra_lang_t
ra_lang_open(const char *pathname)
{
	struct ra_lang *lang;

	if (!pathname || !*pathname) {
		RA_TRACE("invalid arguments");
		return NULL;
	}
	if (!(lang = malloc(sizeof (struct ra_lang)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(lang, 0, sizeof (struct ra_lang));
	if (!(lang->parser = ra_parser_open(pathname))) {
		ra_lang_close(lang);
		RA_TRACE("^");
		return NULL;
	}
	if (ra_evaluate(ra_lang_root(lang))) {
		ra_lang_close(lang);
		RA_TRACE("^");
		return NULL;
	}
	return lang;
}

void
ra_lang_close(ra_lang_t lang)
{
	if (lang) {
		ra_parser_close(lang->parser);
		memset(lang, 0, sizeof (struct ra_lang));
		RA_FREE(lang);
	}
}

struct ra_lang_node *
ra_lang_root(ra_lang_t lang)
{
	assert( lang );

	return ra_parser_root(lang->parser);
}

const char * const RA_LANG_STR[] = {
	"",
	"EXPR_INT",
	"EXPR_REAL",
	"EXPR_STRING",
	"EXPR_IDENTIFIER",
	"EXPR_ARRAY",
	"EXPR_FUNCTION",
	"EXPR_FIELD",
	"EXPR_INC",
	"EXPR_DEC",
	"EXPR_NEG",
	"EXPR_NOT",
	"EXPR_LOGIC_NOT",
	"EXPR_MUL",
	"EXPR_DIV",
	"EXPR_MOD",
	"EXPR_ADD",
	"EXPR_SUB",
	"EXPR_SHL",
	"EXPR_SHR",
	"EXPR_LT",
	"EXPR_GT",
	"EXPR_LE",
	"EXPR_GE",
	"EXPR_EQ",
	"EXPR_NE",
	"EXPR_AND",
	"EXPR_XOR",
	"EXPR_OR",
	"EXPR_LOGIC_AND",
	"EXPR_LOGIC_OR",
	"EXPR_COND",
	"EXPR_LIST",
	"END"
};
