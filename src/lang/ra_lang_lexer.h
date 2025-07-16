/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_lang_lexer.h
 */

#ifndef _RA_LANG_LEXER_H_
#define _RA_LANG_LEXER_H_

#include "ra_lang.h"

typedef struct ra__lang_lexer *ra__lang_lexer_t;

ra__lang_lexer_t ra__lang_lexer_open(const char *s);

void ra__lang_lexer_close(ra__lang_lexer_t lexer);

const struct ra__lang_token *
ra__lang_lexer_lookup(ra__lang_lexer_t lexer, uint64_t i);

uint64_t ra__lang_lexer_size(ra__lang_lexer_t lexer);

#endif // _RA_LANG_LEXER_H_
