/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_lang_parser.h
 */

#ifndef _RA_LANG_PARSER_H_
#define _RA_LANG_PARSER_H_

#include "ra_lang.h"

typedef struct ra__lang_parser *ra__lang_parser_t;

ra__lang_parser_t ra__lang_parser_open(const char *pathname);

void ra__lang_parser_close(ra__lang_parser_t parser);

struct ra__lang_node *ra__lang_parser_root(ra__lang_parser_t parser);

#endif // _RA_LANG_PARSER_H_
