/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_evaluate.h"

#define ERROR(n, f, a)						\
	do {							\
		ra_printf(RA_COLOR_RED_BOLD, "error: ");	\
		ra_printf(RA_COLOR_BLACK_BOLD,			\
			  "%s:%u:%u: " f "\n",			\
			  (n)->token->pathname,			\
			  (n)->token->lineno,			\
			  (n)->token->column,			\
			  (a));					\
		RA_TRACE("parser error");			\
	}							\
	while (0)

static int
expr_int(struct ra_lang_node *node)
{
	node->type = RA_LANG_TYPE_INT;
	return 0;
}

static int
expr_real(struct ra_lang_node *node)
{
	node->type = RA_LANG_TYPE_REAL;
	return 0;
}

static int
expr_string(struct ra_lang_node *node)
{
	node->type = RA_LANG_TYPE_STRING;
	return 0;
}

static int
expr_identifier(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_array(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_function(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_field(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_inc(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_dec(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_neg(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_not(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_logic_not(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_mul(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_div(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_mod(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_add(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_sub(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_shl(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_shr(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_lt(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_gt(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_le(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_ge(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_eq(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_ne(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_and(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_xor(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_or(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_logic_and(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_logic_or(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
expr_cond(struct ra_lang_node *node)
{
	(void)node;
	return 0;
}

static int
evaluate(struct ra_lang_node *node)
{
	int (* const FTBL[])(struct ra_lang_node *) = {
		expr_int,
		expr_real,
		expr_string,
		expr_identifier,
		expr_array,
		expr_function,
		expr_field,
		expr_inc,
		expr_dec,
		expr_neg,
		expr_not,
		expr_logic_not,
		expr_mul,
		expr_div,
		expr_mod,
		expr_add,
		expr_sub,
		expr_shl,
		expr_shr,
		expr_lt,
		expr_gt,
		expr_le,
		expr_ge,
		expr_eq,
		expr_ne,
		expr_and,
		expr_xor,
		expr_or,
		expr_logic_and,
		expr_logic_or,
		expr_cond
	};

	if (node) {
		if ((RA_LANG_EXPR_INT > node->op) ||
		    (RA_LANG_EXPR_COND < node->op)) {
			ERROR(node, "software error detected (aborting)", "");
			abort();
		}
		if (evaluate(node->cond) ||
		    evaluate(node->left) ||
		    evaluate(node->right)) {
			RA_TRACE("^");
			return -1;
		}
		if (FTBL[node->op - RA_LANG_EXPR_INT](node)) {
			RA_TRACE("^");
			return -1;
		}
	}
	return 0;
}

int
ra_evaluate(struct ra_lang_node *node)
{
	if (evaluate(node)) {
		RA_TRACE("^");
		return -1;
	}
	return 0;
}
