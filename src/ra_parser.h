/* Copyright (c) Tony Givargis, 2024-2026 */

#ifndef __RA_PARSER_H__
#define __RA_PARSER_H__

#include "ra_lexer.h"

enum {
	RA_PARSER_,
	RA_PARSER_EXPR_INT,
	RA_PARSER_EXPR_REAL,
	RA_PARSER_EXPR_STRING,
	RA_PARSER_EXPR_IDENTIFIER,
	RA_PARSER_EXPR_ARRAY,     /* left [ right ] */
	RA_PARSER_EXPR_FUNCTION,  /* left ( right ) */
	RA_PARSER_EXPR_FIELD,     /* left . u.s */
	RA_PARSER_EXPR_INC,       /* left ++, ++ right */
	RA_PARSER_EXPR_DEC,       /* left --, -- right */
	RA_PARSER_EXPR_NEG,       /* - right */
	RA_PARSER_EXPR_NOT,       /* ~ right */
	RA_PARSER_EXPR_LOGIC_NOT, /* ! right */
	RA_PARSER_EXPR_MUL,       /* left *  right */
	RA_PARSER_EXPR_DIV,       /* left /  right */
	RA_PARSER_EXPR_MOD,       /* left %  right */
	RA_PARSER_EXPR_ADD,       /* left +  right */
	RA_PARSER_EXPR_SUB,       /* left -  right */
	RA_PARSER_EXPR_SHL,       /* left << right */
	RA_PARSER_EXPR_SHR,       /* left >> right */
	RA_PARSER_EXPR_LT,        /* left <  right */
	RA_PARSER_EXPR_GT,        /* left >  right */
	RA_PARSER_EXPR_LE,        /* left <= right */
	RA_PARSER_EXPR_GE,        /* left >= right */
	RA_PARSER_EXPR_EQ,        /* left == right */
	RA_PARSER_EXPR_NE,        /* left != right */
	RA_PARSER_EXPR_AND,       /* left &  right */
	RA_PARSER_EXPR_XOR,       /* left ^  right */
	RA_PARSER_EXPR_OR,        /* left |  right */
	RA_PARSER_EXPR_LOGIC_AND, /* left && right */
	RA_PARSER_EXPR_LOGIC_OR,  /* left || right */
	RA_PARSER_EXPR_COND,      /* cond ? left : right */
	RA_PARSER_EXPR_LIST,      /* expr: cond, link: right */
	RA_PARSER_END
};

struct ra_parser_node {
	int id;
	int op;
	struct ra_parser_node *cond;
	struct ra_parser_node *left;
	struct ra_parser_node *right;
	const struct ra_lexer_token *token;
};

typedef struct ra_parser *ra_parser_t;

extern const char * const RA_PARSER_STR[];

ra_parser_t ra_parser_open(const char *pathname);

void ra_parser_close(ra_parser_t parser);

struct ra_parser_node *ra_parser_root(ra_parser_t parser);

int ra_parser_print(ra_parser_t parser);

#endif /* __RA_PARSER_H__ */
