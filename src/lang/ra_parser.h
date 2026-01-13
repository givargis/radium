/* Copyright (c) Tony Givargis, 2024-2026 */

#ifndef __RA_PARSER_H__
#define __RA_PARSER_H__

#include "ra_lang.h"
#include "ra_lexer.h"

typedef struct ra_parser *ra_parser_t;

ra_parser_t ra_parser_open(const char *pathname);

void ra_parser_close(ra_parser_t parser);

struct ra_lang_node *ra_parser_root(ra_parser_t parser);

#endif /* __RA_PARSER_H__ */
