/* Copyright (c) Tony Givargis, 2024-2026 */

#ifndef __RA_LANG_H__
#define __RA_LANG_H__

#include "ra_lexer.h"

enum {
	RA_LANG_,
	RA_LANG_EXPR_INT,
	RA_LANG_EXPR_REAL,
	RA_LANG_EXPR_STRING,
	RA_LANG_EXPR_IDENTIFIER,
	RA_LANG_EXPR_ARRAY,     /* left [ right ] */
	RA_LANG_EXPR_FUNCTION,  /* left ( right ) */
	RA_LANG_EXPR_FIELD,     /* left . u.s */
	RA_LANG_EXPR_INC,       /* left ++, ++ right */
	RA_LANG_EXPR_DEC,       /* left --, -- right */
	RA_LANG_EXPR_NEG,       /* - right */
	RA_LANG_EXPR_NOT,       /* ~ right */
	RA_LANG_EXPR_LOGIC_NOT, /* ! right */
	RA_LANG_EXPR_MUL,       /* left *  right */
	RA_LANG_EXPR_DIV,       /* left /  right */
	RA_LANG_EXPR_MOD,       /* left %  right */
	RA_LANG_EXPR_ADD,       /* left +  right */
	RA_LANG_EXPR_SUB,       /* left -  right */
	RA_LANG_EXPR_SHL,       /* left << right */
	RA_LANG_EXPR_SHR,       /* left >> right */
	RA_LANG_EXPR_LT,        /* left <  right */
	RA_LANG_EXPR_GT,        /* left >  right */
	RA_LANG_EXPR_LE,        /* left <= right */
	RA_LANG_EXPR_GE,        /* left >= right */
	RA_LANG_EXPR_EQ,        /* left == right */
	RA_LANG_EXPR_NE,        /* left != right */
	RA_LANG_EXPR_AND,       /* left &  right */
	RA_LANG_EXPR_XOR,       /* left ^  right */
	RA_LANG_EXPR_OR,        /* left |  right */
	RA_LANG_EXPR_LOGIC_AND, /* left && right */
	RA_LANG_EXPR_LOGIC_OR,  /* left || right */
	RA_LANG_EXPR_COND,      /* cond ? left : right */
	RA_LANG_EXPR_LIST,      /* expr: cond, link: right */
	RA_LANG_END
};

enum {
	RA_LANG_TYPE_INT,
	RA_LANG_TYPE_REAL,
	RA_LANG_TYPE_STRING
};

struct ra_lang_node {
	int id;
	int op;
	int type;
	struct ra_lang_node *cond;
	struct ra_lang_node *left;
	struct ra_lang_node *right;
	const struct ra_lexer_token *token;
};

typedef struct ra_lang *ra_lang_t;

extern const char * const RA_LANG_STR[];

ra_lang_t ra_lang_open(const char *pathname);

void ra_lang_close(ra_lang_t lang);

struct ra_lang_node *ra_lang_root(ra_lang_t lang);

#endif /* __RA_LANG_H__ */
