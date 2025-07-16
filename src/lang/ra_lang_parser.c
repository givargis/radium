/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_lang_parser.c
 */

#include "ra_lang_lexer.h"
#include "ra_lang_parser.h"

#define TRACE(p,m,x)						\
	do {							\
		if (!(p)->error) {				\
			ra__log("error: %u:%u: " m,		\
				next((p))->lineno,		\
				next((p))->column,		\
				(x));				\
			(p)->error = 1;				\
			RA__ERROR_TRACE(RA__ERROR_SYNTAX);	\
		}						\
	}							\
	while (0)

#define MKN(p,n,o)						\
	do {							\
		if (!((n) = allocate((p)))) {			\
			TRACE((p), "out of memory", "");	\
			return NULL;				\
		}						\
		(n)->id = ++(p)->id;				\
		(n)->op = (o);					\
		(n)->token = next((p));				\
	}							\
	while (0)

struct ra__lang_parser {
	void *chunk;
	uint64_t size;
	/*-*/
	int id;
	int error;
	uint64_t i, n;
	ra__lang_lexer_t lexer;
	struct ra__lang_node *root;
};

static struct ra__lang_node *
allocate(struct ra__lang_parser *parser)
{
	const uint64_t CHUNK_SIZE = 8192;
	struct ra__lang_node *node;
	uint64_t size;
	void *chunk;

	size = sizeof (struct ra__lang_node);
	if (!parser->chunk || (CHUNK_SIZE < (parser->size + size))) {
		if (!(chunk = ra__malloc(CHUNK_SIZE))) {
			RA__ERROR_TRACE(0);
			return NULL;
		}
		(*((void **)chunk)) = parser->chunk;
		parser->size = size;
		parser->chunk = chunk;
	}
	node = (struct ra__lang_node *)((char *)parser->chunk + parser->size);
	memset(node, 0, size);
	parser->size += size;
	return node;
}

static const struct ra__lang_token *
next(const struct ra__lang_parser *parser)
{
	if (!parser->error && (parser->i < parser->n)) {
		return ra__lang_lexer_lookup(parser->lexer, parser->i);
	}
	return ra__lang_lexer_lookup(parser->lexer, parser->n - 1);
}

static int
match(const struct ra__lang_parser *parser, int op)
{
	const struct ra__lang_token *token;

	if ((token = next(parser)) && (op == token->op)) {
		return 1;
	}
	return 0;
}

static void
forward(struct ra__lang_parser *parser)
{
	if (parser->i < parser->n) {
		++parser->i;
	}
}

/**============================================================================
 * (E1)  expr_primary
 * (E2)  expr_postfix
 * (E3)  expr_unary
 * (E4)  expr_cast
 * (E5)  expr_multiplicative
 * (E6)  expr_additive
 * (E7)  expr_shift
 * (E8)  expr_relational
 * (E9)  expr_equality
 * (E10) expr_and
 * (E11) expr_xor
 * (E12) expr_or
 * (E13) expr_logic_and
 * (E14) expr_logic_or
 * (E15) expr_cond
 * (E16) expr_asn
 * (E17) expr
 *===========================================================================*/

static struct ra__lang_node *expr(struct ra__lang_parser *parser);
static struct ra__lang_node *expr_cast(struct ra__lang_parser *parser);
static struct ra__lang_node *type_name(struct ra__lang_parser *parser);

/**
 * (E1)
 *
 * expr_primary : INT
 *              | REAL
 *              | CHAR
 *              | STRING
 *              | IDENTIFIER
 *              | '(' expr ')'
 */

static struct ra__lang_node *
expr_primary(struct ra__lang_parser *parser)
{
	struct ra__lang_node *node;

	node = NULL;
	if (match(parser, RA__LANG_INT) ||
	    match(parser, RA__LANG_REAL) ||
	    match(parser, RA__LANG_CHAR) ||
	    match(parser, RA__LANG_STRING)) {
		MKN(parser, node, RA__LANG_NODE_EXPR_LITERAL);
		forward(parser);
	}
	else if (match(parser, RA__LANG_IDENTIFIER)) {
		MKN(parser, node, RA__LANG_NODE_EXPR_IDENTIFIER);
		forward(parser);
	}
	else if (match(parser, RA__LANG_OPERATOR_OPEN_PARENTH)) {
		forward(parser);
		if (!(node = expr(parser))) {
			TRACE(parser, "missing expression after '('", "");
			return NULL;
		}
		if (!match(parser, RA__LANG_OPERATOR_CLOSE_PARENTH)) {
			TRACE(parser, "missing ')'", "");
			return NULL;
		}
		forward(parser);
	}
	return node;
}

/**
 * (E2)
 *
 * expr_postfix_ : '++' expr_postfix_
 *               | '--' expr_postfix_
 *               | '(' expr ')' expr_postfix_
 *               | '[' expr ']' expr_postfix_
 *               | '.' IDENTIFIER expr_postfix_
 *               | '->' IDENTIFIER expr_postfix_
 *               | <e>
 *
 * expr_postfix : expr_primary expr_postfix_
 */

static struct ra__lang_node *
expr_postfix_(struct ra__lang_parser *parser, struct ra__lang_node *left)
{
	struct ra__lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA__LANG_OPERATOR_INC)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_INC);
			node->left = left;
			forward(parser);
		}
		else if (match(parser, RA__LANG_OPERATOR_DEC)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_DEC);
			node->left = left;
			forward(parser);
		}
		else if (match(parser, RA__LANG_OPERATOR_OPEN_PARENTH)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_CALL);
			node->left = left;
			forward(parser);
			node->right = expr(parser);
			if (!match(parser,
				   RA__LANG_OPERATOR_CLOSE_PARENTH)) {
				TRACE(parser, "missing ']'", "");
				return NULL;
			}
			forward(parser);
		}
		else if (match(parser, RA__LANG_OPERATOR_OPEN_BRACKET)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_ARRAY);
			node->left = left;
			forward(parser);
			if (!(node->right = expr(parser))) {
				TRACE(parser, "missing '[]' expression", "");
				return NULL;
			}
			if (!match(parser,
				   RA__LANG_OPERATOR_CLOSE_BRACKET)) {
				TRACE(parser, "missing ']'", "");
				return NULL;
			}
			forward(parser);
		}
		else if (match(parser, RA__LANG_OPERATOR_DOT)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_REF);
			node->left = left;
			forward(parser);
			if (!match(parser, RA__LANG_IDENTIFIER)) {
				TRACE(parser, "missing identifier", "");
				return NULL;
			}
			node->token = next(parser);
			forward(parser);
		}
		else if (match(parser, RA__LANG_OPERATOR_POINTER)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_PREF);
			node->left = left;
			forward(parser);
			if (!match(parser, RA__LANG_IDENTIFIER)) {
				TRACE(parser, "missing identifier", "");
				return NULL;
			}
			node->token = next(parser);
			forward(parser);
		}
		else {
			break;
		}
		if (!(node = expr_postfix_(parser, node))) {
			RA__ERROR_TRACE(0);
			return NULL;
		}
	}
	return node;
}

static struct ra__lang_node *
expr_postfix(struct ra__lang_parser *parser)
{
	struct ra__lang_node *node;

	if (!(node = expr_primary(parser))) {
		return NULL;
	}
	return expr_postfix_(parser, node);
}

/**
 * (E3)
 *
 * expr_unary : [ '+' '-' '~' '!' '&' '*' ] expr_cast
 *            | [ '++' '--' ] expr_unary
 *            | SIZEOF expr_unary
 *            | SIZEOF '(' type_name ')'
 *            | exp_postfix
 */

static struct ra__lang_node *
expr_unary(struct ra__lang_parser *parser)
{
	struct ra__lang_node *node;
	uint64_t checkpoint;
	int op;

	node = NULL;
	if (match(parser, RA__LANG_OPERATOR_ADD)) {
		forward(parser);
		if (!(node = expr_cast(parser))) {
			TRACE(parser, "missing unary '+' operand", "");
			return NULL;
		}
	}
	else if (match(parser, RA__LANG_OPERATOR_SUB)) {
		MKN(parser, node, RA__LANG_NODE_EXPR_NEG);
		forward(parser);
		if (!(node->right = expr_cast(parser))) {
			TRACE(parser, "missing unary '-' operand", "");
			return NULL;
		}
	}
	else if (match(parser, RA__LANG_OPERATOR_NOT)) {
		MKN(parser, node, RA__LANG_NODE_EXPR_NOT);
		forward(parser);
		if (!(node->right = expr_cast(parser))) {
			TRACE(parser, "missing '~' operand", "");
			return NULL;
		}
	}
	else if (match(parser, RA__LANG_OPERATOR_LOGIC_NOT)) {
		MKN(parser, node, RA__LANG_NODE_EXPR_LOGIC_NOT);
		forward(parser);
		if (!(node->right = expr_cast(parser))) {
			TRACE(parser, "missing '!' operand", "");
			return NULL;
		}
	}
	else if (match(parser, RA__LANG_OPERATOR_AND)) {
		MKN(parser, node, RA__LANG_NODE_EXPR_ADDRESS);
		forward(parser);
		if (!(node->right = expr_cast(parser))) {
			TRACE(parser, "missing '&' operand", "");
			return NULL;
		}
	}
	else if (match(parser, RA__LANG_OPERATOR_MUL)) {
		MKN(parser, node, RA__LANG_NODE_EXPR_DEREF);
		forward(parser);
		if (!(node->right = expr_cast(parser))) {
			TRACE(parser, "missing '*' operand", "");
			return NULL;
		}
	}
	else if (match(parser, RA__LANG_OPERATOR_INC)) {
		MKN(parser, node, RA__LANG_NODE_EXPR_INC);
		forward(parser);
		if (!(node->right = expr_unary(parser))) {
			TRACE(parser, "missing '++' operand", "");
			return NULL;
		}
	}
	else if (match(parser, RA__LANG_OPERATOR_DEC)) {
		MKN(parser, node, RA__LANG_NODE_EXPR_DEC);
		forward(parser);
		if (!(node->right = expr_unary(parser))) {
			TRACE(parser, "missing '--' operand", "");
			return NULL;
		}
	}
	else if (match(parser, RA__LANG_KEYWORD_SIZEOF)) {
		MKN(parser, node, RA__LANG_NODE_EXPR_SIZEOF);
		forward(parser);
		if (match(parser, RA__LANG_OPERATOR_OPEN_PARENTH)) {
			checkpoint = parser->i;
			forward(parser);
			if (!(node->right = type_name(parser))) {
				parser->i = checkpoint;
			}
			else {
				op = RA__LANG_OPERATOR_CLOSE_PARENTH;
				if (!match(parser, op)) {
					TRACE(parser, "missing ')'", "");
					return NULL;
				}
				forward(parser);
			}
		}
		if (!node->right) {
			if (!(node->right = expr_unary(parser))) {
				TRACE(parser, "missing '*' operand", "");
				return NULL;
			}
		}
	}
	else {
		node = expr_postfix(parser);
	}
	return node;
}

/**
 * (E4)
 *
 * expr_cast : '(' type_name ')' expr_cast
 *           | expr_unary
 */

static struct ra__lang_node *
expr_cast(struct ra__lang_parser *parser)
{
	struct ra__lang_node *node;
	uint64_t checkpoint;

	node = NULL;
	if (match(parser, RA__LANG_OPERATOR_OPEN_PARENTH)) {
		checkpoint = parser->i;
		MKN(parser, node, RA__LANG_NODE_EXPR_CAST);
		forward(parser);
		if (!(node->left = type_name(parser))) {
			parser->i = checkpoint;
			return NULL;
		}
		if (!match(parser, RA__LANG_OPERATOR_CLOSE_PARENTH)) {
			TRACE(parser, "missing ')'", "");
			return NULL;
		}
		forward(parser);
		if (!(node->right = expr_cast(parser))) {
			TRACE(parser, "missing cast operand", "");
			return NULL;
		}
	}
	else {
		node = expr_unary(parser);
	}
	return node;
}

/**
 * (E5)
 *
 * expr_multiplicative_ : [ '*' '/' '%' ] expr_cast expr_multiplicative_
 *                      | <e>
 *
 * expr_multiplicative : expr_cast expr_multiplicative_
 */

static struct ra__lang_node *
expr_multiplicative_(struct ra__lang_parser *parser,
		     struct ra__lang_node *left)
{
	const char * const TBL[] = { "*", "/", "%" };
	struct ra__lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA__LANG_OPERATOR_MUL)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_MUL);
			node->left = left;
			forward(parser);
		}
		else if (match(parser, RA__LANG_OPERATOR_DIV)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_DIV);
			node->left = left;
			forward(parser);
		}
		else if (match(parser, RA__LANG_OPERATOR_MOD)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_MOD);
			node->left = left;
			forward(parser);
		}
		else {
			break;
		}
		if (!(node->right = expr_cast(parser))) {
			TRACE(parser,
			      "missing '%s' operand",
			      TBL[node->op - RA__LANG_NODE_EXPR_MUL]);
			return NULL;
		}
		if (!(node = expr_multiplicative_(parser, node))) {
			RA__ERROR_TRACE(0);
			return NULL;
		}
	}
	return node;
}

static struct ra__lang_node *
expr_multiplicative(struct ra__lang_parser *parser)
{
	struct ra__lang_node *node;

	if (!(node = expr_cast(parser))) {
		return NULL;
	}
	return expr_multiplicative_(parser, node);
}

/**
 * (E6)
 *
 * expr_additive_ : [ '+' '-' ] expr_multiplicative expr_additive_
 *                | <e>
 *
 * expr_additive : expr_multiplicative expr_additive_
 */

static struct ra__lang_node *
expr_additive_(struct ra__lang_parser *parser, struct ra__lang_node *left)
{
	const char * const TBL[] = { "+", "-" };
	struct ra__lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA__LANG_OPERATOR_ADD)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_ADD);
			node->left = left;
			forward(parser);
		}
		else if (match(parser, RA__LANG_OPERATOR_SUB)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_SUB);
			node->left = left;
			forward(parser);
		}
		else {
			break;
		}
		if (!(node->right = expr_multiplicative(parser))) {
			TRACE(parser,
			      "missing '%s' operand",
			      TBL[node->op - RA__LANG_NODE_EXPR_ADD]);
			return NULL;
		}
		if (!(node = expr_additive_(parser, node))) {
			RA__ERROR_TRACE(0);
			return NULL;
		}
	}
	return node;
}

static struct ra__lang_node *
expr_additive(struct ra__lang_parser *parser)
{
	struct ra__lang_node *node;

	if (!(node = expr_multiplicative(parser))) {
		return NULL;
	}
	return expr_additive_(parser, node);
}

/**
 * (E7)
 *
 * expr_shift_ : [ '<<' '>>' ] expr_additive expr_shift_
 *             | <e>
 *
 * expr_shift : expr_additive expr_shift_
 */

static struct ra__lang_node *
expr_shift_(struct ra__lang_parser *parser, struct ra__lang_node *left)
{
	const char * const TBL[] = { "<<", ">>" };
	struct ra__lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA__LANG_OPERATOR_SHL)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_SHL);
			node->left = left;
			forward(parser);
		}
		else if (match(parser, RA__LANG_OPERATOR_SHR)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_SHR);
			node->left = left;
			forward(parser);
		}
		else {
			break;
		}
		if (!(node->right = expr_additive(parser))) {
			TRACE(parser,
			      "missing '%s' operand",
			      TBL[node->op - RA__LANG_NODE_EXPR_SHL]);
			return NULL;
		}
		if (!(node = expr_shift_(parser, node))) {
			RA__ERROR_TRACE(0);
			return NULL;
		}
	}
	return node;
}

static struct ra__lang_node *
expr_shift(struct ra__lang_parser *parser)
{
	struct ra__lang_node *node;

	if (!(node = expr_additive(parser))) {
		return NULL;
	}
	return expr_shift_(parser, node);
}

/**
 * (E8)
 *
 * expr_relational_ : [ '<' '>' '<=' '>=' ] expr_shift expr_relational_
 *                  | <e>
 *
 * expr_relational : expr_shift expr_relational_
 */

static struct ra__lang_node *
expr_relational_(struct ra__lang_parser *parser,
		 struct ra__lang_node *left)
{
	const char * const TBL[] = { "<", ">", "<=", ">=" };
	struct ra__lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA__LANG_OPERATOR_LT)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_LT);
			node->left = left;
			forward(parser);
		}
		else if (match(parser, RA__LANG_OPERATOR_GT)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_GT);
			node->left = left;
			forward(parser);
		}
		else if (match(parser, RA__LANG_OPERATOR_LE)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_LE);
			node->left = left;
			forward(parser);
		}
		else if (match(parser, RA__LANG_OPERATOR_GE)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_GE);
			node->left = left;
			forward(parser);
		}
		else {
			break;
		}
		if (!(node->right = expr_shift(parser))) {
			TRACE(parser,
			      "missing '%s' operand",
			      TBL[node->op - RA__LANG_NODE_EXPR_LT]);
			return NULL;
		}
		if (!(node = expr_relational_(parser, node))) {
			RA__ERROR_TRACE(0);
			return NULL;
		}
	}
	return node;
}

static struct ra__lang_node *
expr_relational(struct ra__lang_parser *parser)
{
	struct ra__lang_node *node;

	if (!(node = expr_shift(parser))) {
		return NULL;
	}
	return expr_relational_(parser, node);
}

/**
 * (E9)
 *
 * expr_equality_ : [ '==' '!=' ] expr_relational expr_equality_
 *                | <e>
 *
 * expr_equality : expr_relational expr_equality_
 */

static struct ra__lang_node *
expr_equality_(struct ra__lang_parser *parser, struct ra__lang_node *left)
{
	const char * const TBL[] = { "==", "!=" };
	struct ra__lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA__LANG_OPERATOR_EQ)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_EQ);
			node->left = left;
			forward(parser);
		}
		else if (match(parser, RA__LANG_OPERATOR_NE)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_NE);
			node->left = left;
			forward(parser);
		}
		else {
			break;
		}
		if (!(node->right = expr_relational(parser))) {
			TRACE(parser,
			      "missing '%s' operand",
			      TBL[node->op - RA__LANG_NODE_EXPR_EQ]);
			return NULL;
		}
		if (!(node = expr_equality_(parser, node))) {
			RA__ERROR_TRACE(0);
			return NULL;
		}
	}
	return node;
}

static struct ra__lang_node *
expr_equality(struct ra__lang_parser *parser)
{
	struct ra__lang_node *node;

	if (!(node = expr_relational(parser))) {
		return NULL;
	}
	return expr_equality_(parser, node);
}

/**
 * (E10)
 *
 * expr_and_ : '&' expr_equality expr_and_
 *           | <e>
 *
 * expr_and : expr_equality expr_and_
 */

static struct ra__lang_node *
expr_and_(struct ra__lang_parser *parser, struct ra__lang_node *left)
{
	struct ra__lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA__LANG_OPERATOR_AND)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_AND);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_equality(parser))) {
				TRACE(parser, "missing '&' operand", "");
				return NULL;
			}
			if (!(node = expr_and_(parser, node))) {
				RA__ERROR_TRACE(0);
				return NULL;
			}
		}
		else {
			break;
		}
	}
	return node;
}

static struct ra__lang_node *
expr_and(struct ra__lang_parser *parser)
{
	struct ra__lang_node *node;

	if (!(node = expr_equality(parser))) {
		return NULL;
	}
	return expr_and_(parser, node);
}

/**
 * (E11)
 *
 * expr_xor_ : '^' expr_and expr_xor_
 *           | <e>
 *
 * expr_xor : expr_and expr_xor_
 */

static struct ra__lang_node *
expr_xor_(struct ra__lang_parser *parser, struct ra__lang_node *left)
{
	struct ra__lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA__LANG_OPERATOR_XOR)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_XOR);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_and(parser))) {
				TRACE(parser, "missing '^' operand", "");
				return NULL;
			}
			if (!(node = expr_xor_(parser, node))) {
				RA__ERROR_TRACE(0);
				return NULL;
			}
		}
		else {
			break;
		}
	}
	return node;
}

static struct ra__lang_node *
expr_xor(struct ra__lang_parser *parser)
{
	struct ra__lang_node *node;

	if (!(node = expr_and(parser))) {
		return NULL;
	}
	return expr_xor_(parser, node);
}

/**
 * (E12)
 *
 * expr_or_ : '|' expr_xor expr_or_
 *          | <e>
 *
 * expr_or : expr_xor expr_or_
 */

static struct ra__lang_node *
expr_or_(struct ra__lang_parser *parser, struct ra__lang_node *left)
{
	struct ra__lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA__LANG_OPERATOR_OR)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_OR);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_xor(parser))) {
				TRACE(parser, "missing '|' operand", "");
				return NULL;
			}
			if (!(node = expr_or_(parser, node))) {
				RA__ERROR_TRACE(0);
				return NULL;
			}
		}
		else {
			break;
		}
	}
	return node;
}

static struct ra__lang_node *
expr_or(struct ra__lang_parser *parser)
{
	struct ra__lang_node *node;

	if (!(node = expr_xor(parser))) {
		return NULL;
	}
	return expr_or_(parser, node);
}

/**
 * (E13)
 *
 * expr_logic_and_ : '&&' expr_or expr_logic_and_
 *                 | <e>
 *
 * expr_logic_and : expr_or expr_logic_and_
 */

static struct ra__lang_node *
expr_logic_and_(struct ra__lang_parser *parser, struct ra__lang_node *left)
{
	struct ra__lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA__LANG_OPERATOR_LOGIC_AND)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_LOGIC_AND);
			node->left = left;
			forward(parser);
			if (!(node->right = expr_or(parser))) {
				TRACE(parser, "missing '&&' operand", "");
				return NULL;
			}
			if (!(node = expr_logic_and_(parser, node))) {
				RA__ERROR_TRACE(0);
				return NULL;
			}
		}
		else {
			break;
		}
	}
	return node;
}

static struct ra__lang_node *
expr_logic_and(struct ra__lang_parser *parser)
{
	struct ra__lang_node *node;

	if (!(node = expr_or(parser))) {
		return NULL;
	}
	return expr_logic_and_(parser, node);
}

/**
 * (E14)
 *
 * expr_logic_or_ : '||' expr_logic_and expr_logic_or_
 *                | <e>
 *
 * expr_logic_or : expr_logic_and expr_logic_or_
 */

static struct ra__lang_node *
expr_logic_or_(struct ra__lang_parser *parser, struct ra__lang_node *left)
{
	struct ra__lang_node *node;

	node = left;
	for (;;) {
		if (match(parser, RA__LANG_OPERATOR_LOGIC_OR)) {
			MKN(parser, node, RA__LANG_NODE_EXPR_LOGIC_OR);
			node->left = left;
			forward(parser);
		}
		else {
			break;
		}
		if (!(node->right = expr_logic_and(parser))) {
			TRACE(parser, "missing '||' operand", "");
			return NULL;
		}
		if (!(node = expr_logic_or_(parser, node))) {
			RA__ERROR_TRACE(0);
			return NULL;
		}
	}
	return node;
}

static struct ra__lang_node *
expr_logic_or(struct ra__lang_parser *parser)
{
	struct ra__lang_node *node;

	if (!(node = expr_logic_and(parser))) {
		return NULL;
	}
	return expr_logic_or_(parser, node);
}

/**
 * (E15)
 *
 * expr : expr_logic_or
 *      | expr_logic_or '?' expr ':' expr_cond
 */

static struct ra__lang_node *
expr_cond(struct ra__lang_parser *parser)
{
	struct ra__lang_node *node, *node_;

	if (!(node = expr_logic_or(parser))) {
		return NULL;
	}
	if (match(parser, RA__LANG_OPERATOR_QUESTION)) {
		MKN(parser, node_, RA__LANG_NODE_EXPR_COND);
		node_->cond = node;
		forward(parser);
		if (!(node_->left = expr(parser))) {
			TRACE(parser, "missing '?' operand", "");
			return NULL;
		}
		if (!match(parser, RA__LANG_OPERATOR_COLON)) {
			TRACE(parser, "missing ':'", "");
			return NULL;
		}
		forward(parser);
		if (!(node_->right = expr_cond(parser))) {
			TRACE(parser, "missing ':' operand", "");
			return NULL;
		}
		node = node_;
	}
	return node;
}

/**
 * (E16)
 *
 * expr : expr_unary
 *        [ '=' '+=' '-=' '*=' '/=' '%=' '<<=' '>>=' '|=' '^=' '&=' ] expr_asn
 *      | expr_cond
 */

static struct ra__lang_node *
expr_asn(struct ra__lang_parser *parser)
{
	struct ra__lang_node *node, *node_;
	uint64_t checkpoint;
	int op;

	checkpoint = parser->i;
	if ((node_ = expr_unary(parser))) {
		if (match(parser, RA__LANG_OPERATOR_ASN) ||
		    match(parser, RA__LANG_OPERATOR_ADDASN) ||
		    match(parser, RA__LANG_OPERATOR_SUBASN) ||
		    match(parser, RA__LANG_OPERATOR_MULASN) ||
		    match(parser, RA__LANG_OPERATOR_DIVASN) ||
		    match(parser, RA__LANG_OPERATOR_MODASN) ||
		    match(parser, RA__LANG_OPERATOR_SHLASN) ||
		    match(parser, RA__LANG_OPERATOR_SHRASN) ||
		    match(parser, RA__LANG_OPERATOR_ORASN) ||
		    match(parser, RA__LANG_OPERATOR_XORASN) ||
		    match(parser, RA__LANG_OPERATOR_ANDASN)) {
			op = next(parser)->op - RA__LANG_OPERATOR_ASN;
			MKN(parser, node, RA__LANG_NODE_EXPR_ASN + op);
			node->left = node_;
			forward(parser);
			if (!(node->right = expr_asn(parser))) {
				TRACE(parser,
				      "missing assignment right value",
				      "");
				return NULL;
			}
			return node;
		}
	}
	parser->i = checkpoint;
	return expr_cond(parser);
}

/**
 * (E17)
 *
 * expr : expr_asn { ',' expr_asn }
 */

static struct ra__lang_node *
expr(struct ra__lang_parser *parser)
{
	struct ra__lang_node *node, *head, *tail;
	int mark;

	mark = 0;
	head = tail = NULL;
	while ((node = expr_asn(parser))) {
		if (tail) {
			tail->link = node;
			tail = node;
		}
		else {
			head = node;
			tail = node;
		}
		if (!(mark = match(parser, RA__LANG_OPERATOR_COMMA))) {
			break;
		}
		forward(parser);
	}
	if (mark) {
		TRACE(parser, "dangling ','", "");
		return NULL;
	}
	return head;
}

static struct ra__lang_node *
type_name(struct ra__lang_parser *parser)
{
	struct ra__lang_node *node;

	node = NULL;
	if (match(parser, RA__LANG_KEYWORD_INT)) {
		MKN(parser, node, RA__LANG_NODE_);
		forward(parser);
	}
	return node;
}

static struct ra__lang_node *
top(struct ra__lang_parser *parser)
{
	struct ra__lang_node *node;

	if (match(parser, RA__LANG_EOF)) {
		TRACE(parser, "empty translation unit", "");
		return NULL;
	}
	if (!(node = expr(parser))) {
		TRACE(parser, "syntax error", "");
		return NULL;
	}
	if (!match(parser, RA__LANG_EOF)) {
		TRACE(parser, "syntax error", "");
		return NULL;
	}
	return node;
}

ra__lang_parser_t
ra__lang_parser_open(const char *pathname)
{
	struct ra__lang_parser *parser;

	assert( ra__strlen(pathname) );

	if (!(parser = ra__malloc(sizeof (struct ra__lang_parser)))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(parser, 0, sizeof (struct ra__lang_parser));
	if (!(parser->lexer = ra__lang_lexer_open(pathname)) ||
	    !(parser->n = ra__lang_lexer_size(parser->lexer)) ||
	    !(parser->root = top(parser))) {
		ra__lang_parser_close(parser);
		RA__ERROR_TRACE(0);
		return NULL;
	}
	return parser;
}

void
ra__lang_parser_close(ra__lang_parser_t parser)
{
	void *chunk;

	if (parser) {
		while ((chunk = parser->chunk)) {
			parser->chunk = (*((void **)chunk));
			RA__FREE(chunk);
		}
		ra__lang_lexer_close(parser->lexer);
		memset(parser, 0, sizeof (struct ra__lang_parser));
	}
	RA__FREE(parser);
}

struct ra__lang_node *
ra__lang_parser_root(ra__lang_parser_t parser)
{
	assert( parser );

	return parser->root;
}
