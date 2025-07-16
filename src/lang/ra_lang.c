/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_lang.c
 */

#include "ra_lang_parser.h"
#include "ra_lang.h"

struct ra__lang {
	ra__lang_parser_t parser;
};

ra__lang_t
ra__lang_open(const char *pathname)
{
	struct ra__lang *lang;

	assert( ra__strlen(pathname) );

	if (!(lang = ra__malloc(sizeof (struct ra__lang)))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(lang, 0, sizeof (struct ra__lang));
	if (!(lang->parser = ra__lang_parser_open(pathname))) {
		ra__lang_close(lang);
		RA__ERROR_TRACE(0);
		return NULL;
	}
	return lang;
}

void
ra__lang_close(ra__lang_t lang)
{
	if (lang) {
		ra__lang_parser_close(lang->parser);
		memset(lang, 0, sizeof (struct ra__lang));
	}
	RA__FREE(lang);
}

struct ra__lang_node *
ra__lang_root(ra__lang_t lang)
{
	assert( lang );

	return ra__lang_parser_root(lang->parser);
}
