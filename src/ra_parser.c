/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_parser.h"

#define NLEN ( sizeof (struct ra_parser_node) )

#define ERROR(p, f, a)							\
	do {								\
		if (!(p)->stop) {					\
			(p)->stop = 1;					\
			ra_printf(RA_COLOR_RED_BOLD, "error: ");	\
			ra_printf(RA_COLOR_BLACK_BOLD,			\
				  "%s:%u:%u: " f "\n",			\
				  (p)->pathname,			\
				  next((p))->lineno,			\
				  next((p))->column,			\
				  (a));					\
			RA_TRACE("parser error");			\
		}							\
	}								\
	while (0)

#define MKN(p, d, o)							\
	do {								\
		if (!((d) = ra_vector_append((p)->nodes, NLEN))) {	\
			ERROR((p), "out of memory", "");		\
			return NULL;					\
		}							\
		(d)->id = ++(p)->id;					\
		(d)->op = (o);						\
		(d)->token = next((p));					\
	}								\
	while (0)

struct ra_parser {
	int id;
	int stop;
	uint64_t i, n;
	ra_lexer_t lexer;
	ra_vector_t nodes;
	const char *pathname;
	struct ra_parser_node *root;
};

static const struct ra_lexer_token *
next(const struct ra_parser *parser)
{
	if (!parser->stop && (parser->i < parser->n)) {
		return ra_lexer_lookup(parser->lexer, parser->i);
	}
	return ra_lexer_lookup(parser->lexer, parser->n - 1);
}

static int /* BOOL */
match(const struct ra_parser *parser, int op)
{
	const struct ra_lexer_token *token;

	if ((token = next(parser)) && (op == token->op)) {
		return 1;
	}
	return 0;
}

static void
forward(struct ra_parser *parser)
{
	if (parser->i < parser->n) {
		++parser->i;
	}
}

/**============================================================================
 * (E1)  expr_primary
 * (E2)  expr_postfix
 * (E3)  expr_unary
 * (E3)  expr_cast
 * (E4)  expr_multiplicative
 * (E5)  expr_additive
 * (E6)  expr_shift
 * (E7)  expr_relational
 * (E8)  expr_equality
 * (E9)  expr_and
 * (E10) expr_xor
 * (E11) expr_or
 * (E12) expr_logic_and
 * (E13) expr_logic_xor_or
 * (E??) expr_list
 * (E14) expr
 *===========================================================================*/

static struct ra_parser_node *expr(struct ra_parser *parser);
static struct ra_parser_node *expr_list(struct ra_parser *parser);

/**
 * (E1)
 *
 * expr_primary :
 *              | INT
 *              | REAL
 *              | STRING
 *              | IDENTIFIER
 *              | '(' expr ')'
 */

static struct ra_parser_node *
expr_primary(struct ra_parser *parser)
{
	struct ra_parser_node *node;

	node = NULL;
	if (match(parser, RA_LEXER_INT)) {
		MKN(parser, node, RA_PARSER_EXPR_INT);
		forward(parser);
	}
	else if (match(parser, RA_LEXER_REAL)) {
		MKN(parser, node, RA_PARSER_EXPR_REAL);
		forward(parser);
	}
	else if (match(parser, RA_LEXER_STRING)) {
		MKN(parser, node, RA_PARSER_EXPR_STRING);
		forward(parser);
	}
	else if (match(parser, RA_LEXER_IDENTIFIER)) {
		MKN(parser, node, RA_PARSER_EXPR_IDENTIFIER);
		forward(parser);
	}
	else if (match(parser, RA_LEXER_OPERATOR_PARENTH_OPEN)) {
		forward(parser);
		if (!(node = expr(parser))) {
			ERROR(parser, "invalid expression after '('", "");
			return NULL;
		}
		if (!match(parser, RA_LEXER_OPERATOR_PARENTH_CLOSE)) {
			ERROR(parser, "missing ')'", "");
			return NULL;
		}
		forward(parser);
	}
	return node;
}

/**
 * (E2)
 *
 * expr_postfix_ : { '[' expr ']' expr_postfix_ }
 *	         | { '(' expr_list ')' expr_postfix_ }
 *	         | { '.' IDENTIFIER expr_postfix_ }
 *	         | { '->' IDENTIFIER expr_postfix_ }
 *	         | { '++' expr_postfix_ }
 *	         | { '--' expr_postfix_ }
 *               | <e>
 *
 * expr_postfix : expr_primary expr_postfix_
 */

static struct ra_parser_node *
expr_postfix_(struct ra_parser *parser, struct ra_parser_node *left)
{
	struct ra_parser_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA_LEXER_OPERATOR_BRACKET_OPEN)) {
			MKN(parser, node, RA_PARSER_EXPR_ARRAY);
			node->left = left;
			forward(parser);
			if (!(node->right = expr(parser))) {
				ERROR(parser,
				      "missing array index expression",
				      "");
				return NULL;
			}
			if (!match(parser, RA_LEXER_OPERATOR_BRACKET_CLOSE)) {
				ERROR(parser, "missing ']'", "");
				return NULL;
			}
			forward(parser);
		}
		else if (match(parser, RA_LEXER_OPERATOR_PARENTH_OPEN)) {
			MKN(parser, node, RA_PARSER_EXPR_FUNCTION);
			node->left = left;
			forward(parser);
			node->right = expr_list(parser);
			if (!match(parser, RA_LEXER_OPERATOR_PARENTH_CLOSE)) {
				ERROR(parser, "missing ')'", "");
				return NULL;
			}
			forward(parser);
		}
		else if (match(parser, RA_LEXER_OPERATOR_DOT)) {
			MKN(parser, node, RA_PARSER_EXPR_FIELD);
			node->left = left;
			forward(parser);
			if (!match(parser, RA_LEXER_IDENTIFIER)) {
				ERROR(parser, "missing identifier", "");
				return NULL;
			}
			node->token = next(parser);
			forward(parser);
		}
		else if (match(parser, RA_LEXER_OPERATOR_ARROW)) {
			MKN(parser, node, RA_PARSER_EXPR_PFIELD);
			node->left = left;
			forward(parser);
			if (!match(parser, RA_LEXER_IDENTIFIER)) {
				ERROR(parser, "missing identifier", "");
				return NULL;
			}
			node->token = next(parser);
			forward(parser);
		}
		else if (match(parser, RA_LEXER_OPERATOR_INC)) {
			MKN(parser, node, RA_PARSER_EXPR_INC);
			node->left = left;
			forward(parser);
		}
		else if (match(parser, RA_LEXER_OPERATOR_DEC)) {
			MKN(parser, node, RA_PARSER_EXPR_DEC);
			node->left = left;
			forward(parser);
		}
		else {
			break;
		}
		if (!(node = expr_postfix_(parser, node))) {
			RA_TRACE("^");
			return NULL;
		}
	}
	return node;

}

static struct ra_parser_node *
expr_postfix(struct ra_parser *parser)
{
	struct ra_parser_node *node;

	if (!(node = expr_primary(parser))) {
		return NULL;
	}
	return expr_postfix_(parser, node);
}

#if(0)

/**
 * (E3)
 *
 * expr_unary : [ '&' '*' '+' '-' '~' '!' ] expr_cast
 *            | [ '++' '--' ] expr_unary
 *            | 'sizeof' expr_unary
 *            | 'sizeof' '(' type_name ')'
 *            | expr_postfix
 */

static struct ra_parser_node *
expr_unary(struct ra_parser *parser)
{
	struct ra_parser_node *node;

	node = NULL;
	if (match(parser, RA_LEXER_OPERATOR_ADD)) {
		forward(parser);
		if (!(node = expr_cast(parser))) {
			ERROR(parser, "invalid unary '+' operand", "");
			return NULL;
		}
	}
	else if (match(parser, RA_LEXER_OPERATOR_SUB)) {
		MKN(parser, node, RA_PARSER_EXPR_NEG);
		forward(parser);
		if (!(node->right = expr_cast(parser))) {
			ERROR(parser, "invalid unary '-' operand", "");
			return NULL;
		}
	}
	else if (match(parser, RA_LEXER_OPERATOR_NOT)) {
		MKN(parser, node, RA_PARSER_EXPR_NOT);
		forward(parser);
		if (!(node->right = expr_cast(parser))) {
			ERROR(parser, "invalid '~' operand", "");
			return NULL;
		}
	}
	else if (match(parser, RA_LEXER_OPERATOR_LOGIC_NOT)) {
		MKN(parser, node, RA_PARSER_EXPR_LOGIC_NOT);
		forward(parser);
		if (!(node->right = expr_cast(parser))) {
			ERROR(parser, "invalid '!' operand", "");
			return NULL;
		}
	}
	else {
		node = expr_postfix(parser);
	}
	return node;
}

/**
 * (E3)
 *
 * expr_cast : expr_unary
 *           | '(' decl_type ')' expr_cast
 */

struct ra_parser_node *
expr_cast(struct ra_parser *parser)
{
	struct ra_parser_node *node;

	if (match(parser, RA_LEXER_OPERATOR_OPEN_PARENTH)) {
		MKN(parser, node, RA_PARSER_EXPR_CAST);
		forward(parser);
		if (!(node->left = decl_type(parser))) {
			reverse(parser);
			return expr_unary(parser);
		}
		if (!match(parser, RA_LEXER_OPERATOR_CLOSE_PARENTH)) {
			ERROR(parser, "missing ')'", "");
			return NULL;
		}
		forward(parser);
		if (!(node->right = expr_cast(parser))) {
			ERROR(parser, "invalid cast expression", "");
			return NULL;
		}
		return node;
	}
	return expr_unary(parser);
}

/**
 * (E4)
 *
 * expr_multiplicative_ : { [ '*' '/' '%' ] expr_cast expr_multiplicative_ }
 *                      | <e>
 *
 * expr_multiplicative : expr_cast expr_multiplicative_
 */

static struct ra_parser_node *
expr_multiplicative_(struct ra_parser *parser,
		     struct ra_parser_node *left)
{
	struct ra_parser_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA_LEXER_OPERATOR_MUL)) {
			MKN(parser, node, RA_PARSER_EXPR_MUL);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_cast(parser))) {
				ERROR(parser, "invalid '*' operand", "");
				return NULL;
			}
			if (!(node = expr_multiplicative_(parser, node))) {
				P__LOG_DEBUG(0);
				return NULL;
			}
		}
		else if (match(parser, RA_LEXER_OPERATOR_DIV)) {
			MKN(parser, node, RA_PARSER_EXPR_DIV);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_cast(parser))) {
				ERROR(parser, "invalid '/' operand", "");
				return NULL;
			}
			if (!(node = expr_multiplicative_(parser, node))) {
				P__LOG_DEBUG(0);
				return NULL;
			}
		}
		else if (match(parser, RA_LEXER_OPERATOR_MOD)) {
			MKN(parser, node, RA_PARSER_EXPR_MOD);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_cast(parser))) {
				ERROR(parser, "invalid '%%' operand", "");
				return NULL;
			}
			if (!(node = expr_multiplicative_(parser, node))) {
				P__LOG_DEBUG(0);
				return NULL;
			}
		}
		else {
			break;
		}
	}
	return node;
}

static struct ra_parser_node *
expr_multiplicative(struct ra_parser *parser)
{
	struct ra_parser_node *node;

	if (!(node = expr_cast(parser))) {
		return NULL;
	}
	return expr_multiplicative_(parser, node);
}

/**
 * (E5)
 *
 * expr_additive_ : { [ '+' '-' ] expr_multiplicative expr_additive_ }
 *                | <e>
 *
 * expr_additive : expr_multiplicative expr_additive_
 */

static struct ra_parser_node *
expr_additive_(struct ra_parser *parser, struct ra_parser_node *left)
{
	struct ra_parser_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA_LEXER_OPERATOR_ADD)) {
			MKN(parser, node, RA_PARSER_EXPR_ADD);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_multiplicative(parser))) {
				ERROR(parser, "invalid '+' operand", "");
				return NULL;
			}
			if (!(node = expr_additive_(parser, node))) {
				P__LOG_DEBUG(0);
				return NULL;
			}
		}
		else if (match(parser, RA_LEXER_OPERATOR_SUB)) {
			MKN(parser, node, RA_PARSER_EXPR_SUB);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_multiplicative(parser))) {
				ERROR(parser, "invalid '-' operand", "");
				return NULL;
			}
			if (!(node = expr_additive_(parser, node))) {
				P__LOG_DEBUG(0);
				return NULL;
			}
		}
		else {
			break;
		}
	}
	return node;
}

static struct ra_parser_node *
expr_additive(struct ra_parser *parser)
{
	struct ra_parser_node *node;

	if (!(node = expr_multiplicative(parser))) {
		return NULL;
	}
	return expr_additive_(parser, node);
}

/**
 * (E6)
 *
 * expr_shift_ : { [ '<<' '>>' ] expr_additive expr_shift_ }
 *             | <e>
 *
 * expr_shift : expr_additive expr_shift_
 */

static struct ra_parser_node *
expr_shift_(struct ra_parser *parser, struct ra_parser_node *left)
{
	struct ra_parser_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA_LEXER_OPERATOR_SHL)) {
			MKN(parser, node, RA_PARSER_EXPR_SHL);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_additive(parser))) {
				ERROR(parser, "invalid '<<' operand", "");
				return NULL;
			}
			if (!(node = expr_shift_(parser, node))) {
				P__LOG_DEBUG(0);
				return NULL;
			}
		}
		else if (match(parser, RA_LEXER_OPERATOR_SHR)) {
			MKN(parser, node, RA_PARSER_EXPR_SHR);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_additive(parser))) {
				ERROR(parser, "invalid '>>' operand", "");
				return NULL;
			}
			if (!(node = expr_shift_(parser, node))) {
				P__LOG_DEBUG(0);
				return NULL;
			}
		}
		else {
			break;
		}
	}
	return node;
}

static struct ra_parser_node *
expr_shift(struct ra_parser *parser)
{
	struct ra_parser_node *node;

	if (!(node = expr_additive(parser))) {
		return NULL;
	}
	return expr_shift_(parser, node);
}

/**
 * (E7)
 *
 * expr_relational_ : { [ '<' '>' '<=' '>=' ] expr_shift expr_relational_ }
 *                  | <e>
 *
 * expr_relational : expr_shift expr_relational_
 */

static struct ra_parser_node *
expr_relational_(struct ra_parser *parser,
		 struct ra_parser_node *left)
{
	struct ra_parser_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA_LEXER_OPERATOR_LT)) {
			MKN(parser, node, RA_PARSER_EXPR_LT);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_shift(parser))) {
				ERROR(parser, "invalid '<' operand", "");
				return NULL;
			}
			if (!(node = expr_relational_(parser, node))) {
				P__LOG_DEBUG(0);
				return NULL;
			}
		}
		else if (match(parser, RA_LEXER_OPERATOR_GT)) {
			MKN(parser, node, RA_PARSER_EXPR_GT);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_shift(parser))) {
				ERROR(parser, "invalid '>' operand", "");
				return NULL;
			}
			if (!(node = expr_relational_(parser, node))) {
				P__LOG_DEBUG(0);
				return NULL;
			}
		}
		else if (match(parser, RA_LEXER_OPERATOR_LE)) {
			MKN(parser, node, RA_PARSER_EXPR_LE);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_shift(parser))) {
				ERROR(parser, "invalid '<=' operand", "");
				return NULL;
			}
			if (!(node = expr_relational_(parser, node))) {
				P__LOG_DEBUG(0);
				return NULL;
			}
		}
		else if (match(parser, RA_LEXER_OPERATOR_GE)) {
			MKN(parser, node, RA_PARSER_EXPR_GE);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_shift(parser))) {
				ERROR(parser, "invalid '>=' operand", "");
				return NULL;
			}
			if (!(node = expr_relational_(parser, node))) {
				P__LOG_DEBUG(0);
				return NULL;
			}
		}
		else {
			break;
		}
	}
	return node;
}

static struct ra_parser_node *
expr_relational(struct ra_parser *parser)
{
	struct ra_parser_node *node;

	if (!(node = expr_shift(parser))) {
		return NULL;
	}
	return expr_relational_(parser, node);
}

/**
 * (E8)
 *
 * expr_equality_ : { [ '==' '!=' ] expr_relational expr_equality_ }
 *                | <e>
 *
 * expr_equality : expr_relational expr_equality_
 */

static struct ra_parser_node *
expr_equality_(struct ra_parser *parser, struct ra_parser_node *left)
{
	const char * const TBL[] = { "==", "!=", "<=>" };
	struct ra_parser_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA_LEXER_OPERATOR_EQ)) {
			MKN(parser, node, RA_PARSER_EXPR_EQ);
			node->left = left;
			forward(parser);
		}
		else if (match(parser, RA_LEXER_OPERATOR_NE)) {
			MKN(parser, node, RA_PARSER_EXPR_NE);
			node->left = left;
			forward(parser);
		}
		else {
			break;
		}
		if (!(node->right = expr_relational(parser))) {
			ERROR(parser,
			      "invalid %s operand",
			      TBL[node->op - RA_PARSER_EXPR_EQ]);
			return NULL;
		}
		if (!(node = expr_equality_(parser, node))) {
			P__LOG_DEBUG(0);
			return NULL;
		}
	}
	return node;
}

static struct ra_parser_node *
expr_equality(struct ra_parser *parser)
{
	struct ra_parser_node *node;

	if (!(node = expr_relational(parser))) {
		return NULL;
	}
	return expr_equality_(parser, node);
}

/**
 * (E9)
 *
 * expr_and_ : { '&' expr_equality expr_and_ }
 *           | <e>
 *
 * expr_and : expr_equality expr_and_
 */

static struct ra_parser_node *
expr_and_(struct ra_parser *parser, struct ra_parser_node *left)
{
	struct ra_parser_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA_LEXER_OPERATOR_AND)) {
			MKN(parser, node, RA_PARSER_EXPR_AND);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_equality(parser))) {
				ERROR(parser, "invalid '&' operand", "");
				return NULL;
			}
			if (!(node = expr_and_(parser, node))) {
				P__LOG_DEBUG(0);
				return NULL;
			}
		}
		else {
			break;
		}
	}
	return node;
}

static struct ra_parser_node *
expr_and(struct ra_parser *parser)
{
	struct ra_parser_node *node;

	if (!(node = expr_equality(parser))) {
		return NULL;
	}
	return expr_and_(parser, node);
}

/**
 * (E10)
 *
 * expr_xor_ : { '^' expr_and expr_xor_ }
 *           | <e>
 *
 * expr_xor : expr_and expr_xor_
 */

static struct ra_parser_node *
expr_xor_(struct ra_parser *parser, struct ra_parser_node *left)
{
	struct ra_parser_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA_LEXER_OPERATOR_XOR)) {
			MKN(parser, node, RA_PARSER_EXPR_XOR);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_and(parser))) {
				ERROR(parser, "invalid '^' operand", "");
				return NULL;
			}
			if (!(node = expr_xor_(parser, node))) {
				P__LOG_DEBUG(0);
				return NULL;
			}
		}
		else {
			break;
		}
	}
	return node;
}

static struct ra_parser_node *
expr_xor(struct ra_parser *parser)
{
	struct ra_parser_node *node;

	if (!(node = expr_and(parser))) {
		return NULL;
	}
	return expr_xor_(parser, node);
}

/**
 * (E11)
 *
 * expr_or_ : { '|' expr_xor expr_or_ }
 *          | <e>
 *
 * expr_or : expr_xor expr_or_
 */

static struct ra_parser_node *
expr_or_(struct ra_parser *parser, struct ra_parser_node *left)
{
	struct ra_parser_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA_LEXER_OPERATOR_OR)) {
			MKN(parser, node, RA_PARSER_EXPR_OR);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_xor(parser))) {
				ERROR(parser, "invalid '|' operand", "");
				return NULL;
			}
			if (!(node = expr_or_(parser, node))) {
				P__LOG_DEBUG(0);
				return NULL;
			}
		}
		else {
			break;
		}
	}
	return node;
}

static struct ra_parser_node *
expr_or(struct ra_parser *parser)
{
	struct ra_parser_node *node;

	if (!(node = expr_xor(parser))) {
		return NULL;
	}
	return expr_or_(parser, node);
}

/**
 * (E12)
 *
 * expr_logic_and_ : { '&&' expr_or expr_logic_and_ }
 *                 | <e>
 *
 * expr_logic_and : expr_or expr_logic_and_
 */

static struct ra_parser_node *
expr_logic_and_(struct ra_parser *parser,
		struct ra_parser_node *left)
{
	struct ra_parser_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA_LEXER_OPERATOR_LOGIC_AND)) {
			MKN(parser, node, RA_PARSER_EXPR_LOGIC_AND);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_or(parser))) {
				ERROR(parser, "invalid '&&' operand", "");
				return NULL;
			}
			if (!(node = expr_logic_and_(parser, node))) {
				P__LOG_DEBUG(0);
				return NULL;
			}
		}
		else {
			break;
		}
	}
	return node;
}

static struct ra_parser_node *
expr_logic_and(struct ra_parser *parser)
{
	struct ra_parser_node *node;

	if (!(node = expr_or(parser))) {
		return NULL;
	}
	return expr_logic_and_(parser, node);
}

/**
 * (E13)
 *
 * expr_logic_or_ : { [ '^^' '||' ] expr_logic_and expr_logic_xor_or_ }
 *                | <e>
 *
 * expr_logic_xor_or : expr_logic_and expr_logic_xor_or_
 */

static struct ra_parser_node *
expr_logic_or_(struct ra_parser *parser, struct ra_parser_node *left)
{
	const char * const TBL[] = { "^^", "||" };
	struct ra_parser_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA_LEXER_OPERATOR_LOGIC_XOR)) {
			MKN(parser, node, RA_PARSER_EXPR_LOGIC_XOR);
			node->left = left;
			forward(parser);
		}
		else if (match(parser, RA_LEXER_OPERATOR_LOGIC_OR)) {
			MKN(parser, node, RA_PARSER_EXPR_LOGIC_OR);
			node->left = left;
			forward(parser);
		}
		else {
			break;
		}
		if (!(node->right = expr_logic_and(parser))) {
			ERROR(parser,
			      "invalid %s operand",
			      TBL[node->op - RA_PARSER_EXPR_XOR]);
			return NULL;
		}
		if (!(node = expr_logic_or_(parser, node))) {
			P__LOG_DEBUG(0);
			return NULL;
		}
	}
	return node;
}

static struct ra_parser_node *
expr_logic_or(struct ra_parser *parser)
{
	struct ra_parser_node *node;

	if (!(node = expr_logic_and(parser))) {
		return NULL;
	}
	return expr_logic_or_(parser, node);
}

/**
 * (E14)
 *
 * expr_ : expr_logic_or { '?' expr ':' expr }?
 */

static struct ra_parser_node *
expr(struct ra_parser *parser)
{
	struct ra_parser_node *node, *node_;

	assert( parser );

	if (!(node = expr_logic_or(parser))) {
		return NULL;
	}
	if (match(parser, RA_LEXER_OPERATOR_QUESTION)) {
		MKN(parser, node_, RA_PARSER_EXPR_COND);
		node_->cond = node;
		forward(parser);
		if (!(node_->left = expr(parser))) {
			ERROR(parser, "invalid '?' operand", "");
			return NULL;
		}
		if (!match(parser, RA_LEXER_OPERATOR_COLON)) {
			ERROR(parser, "invalid ':'", "");
			return NULL;
		}
		forward(parser);
		if (!(node_->right = expr(parser))) {
			ERROR(parser, "invalid ':' operand", "");
			return NULL;
		}
		node = node_;
	}
	return node;
}

#endif

static struct ra_parser_node *
expr(struct ra_parser *parser)
{
	struct ra_parser_node *node;

	if (!(node = expr_postfix(parser))) {
		return NULL;
	}
	return node;
}

/**
 * (E??)
 *
 * expr_list : expr { ',' expr }
 */

static struct ra_parser_node *
expr_list(struct ra_parser *parser)
{
	struct ra_parser_node *node, *node_, *head, *tail;
	int mark;

	mark = 0;
	head = tail = NULL;
	while ((node_ = expr(parser))) {
		MKN(parser, node, RA_PARSER_EXPR_LIST);
		node->cond = node_;
		if (tail) {
			tail->right = node;
			tail = node;
		}
		else {
			head = node;
			tail = node;
		}
		if (!(mark = match(parser, RA_LEXER_OPERATOR_COMMA))) {
			break;
		}
		forward(parser);
	}
	if (mark) {
		ERROR(parser, "dangling ','", "");
		return NULL;
	}
	return head;
}

static struct ra_parser_node *
top(struct ra_parser *parser)
{
	struct ra_parser_node *node;

	if (!(node = expr(parser))) {
		ERROR(parser, "invalid program", "");
		return NULL;
	}
	if (!match(parser, RA_LEXER_END)) {
		ERROR(parser, "invalid token", "");
		return NULL;
	}
	return node;
}

ra_parser_t
ra_parser_open(const char *pathname)
{
	struct ra_parser *parser;

	assert( pathname && strlen(pathname) );

	if (!(parser = malloc(sizeof (struct ra_parser)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(parser, 0, sizeof (struct ra_parser));
	parser->pathname = pathname;
	if (!(parser->nodes = ra_vector_open()) ||
	    !(parser->lexer = ra_lexer_open(pathname))) {
		ra_parser_close(parser);
		RA_TRACE("^");
		return NULL;
	}
	parser->n = ra_lexer_items(parser->lexer);
	if (!(parser->root = top(parser))) {
		ra_parser_close(parser);
		RA_TRACE("^");
		return NULL;
	}
	return parser;
}

void
ra_parser_close(ra_parser_t parser)
{
	if (parser) {
		ra_lexer_close(parser->lexer);
		ra_vector_close(parser->nodes);
		memset(parser, 0, sizeof (struct ra_parser));
		RA_FREE(parser);
	}
}

struct ra_parser_node *
ra_parser_root(ra_parser_t parser)
{
	assert( parser );

	return parser->root;
}

static void
print(const struct ra_parser_node *node)
{
	char buf[3][32];

	if (node) {
		print(node->left);
		print(node->right);
		print(node->cond);

		ra_sprintf(buf[0],
			   sizeof (buf[0]),
			   "%d",
			   node->left ? node->left->id : 0);
		ra_sprintf(buf[1],
			   sizeof (buf[1]),
			   "%d",
			   node->right ? node->right->id : 0);
		ra_sprintf(buf[2],
			   sizeof (buf[2]),
			   "%d",
			   node->cond ? node->cond->id : 0);

		printf("%2d %2s %2s %2s %s\n",
		       node->id,
		       node->left ? buf[0] : "-",
		       node->right ? buf[1] : "-",
		       node->cond ? buf[2] : "-",
		       RA_PARSER_STR[node->op]);
	}
}

int
ra_parser_print(ra_parser_t parser)
{
	const char *s;
	ra_csv_t csv;

	assert( parser );

	if (!(s = ra_lexer_csv(parser->lexer))) {
		RA_TRACE("^");
		return -1;
	}
	if (!(csv = ra_csv_open(s)) || ra_csv_print(csv)) {
		ra_csv_close(csv);
		RA_FREE(s);
		RA_TRACE("^");
		return -1;
	}
	ra_csv_close(csv);
	RA_FREE(s);

	print(parser->root);

	return 0;
}

const char * const RA_PARSER_STR[] = {
	"",
	"EXPR_INT",
	"EXPR_REAL",
	"EXPR_STRING",
	"EXPR_IDENTIFIER",
	"EXPR_ARRAY",
	"EXPR_FUNCTION",
	"EXPR_FIELD",
	"EXPR_PFIELD",
	"EXPR_INC",
	"EXPR_DEC",
	"EXPR_NEG",
	"EXPR_NOT",
	"EXPR_LOGIC_NOT",
	"EXPR_CAST",
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
