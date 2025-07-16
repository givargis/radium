/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_lang.h
 */

#ifndef _RA_LANG_H_
#define _RA_LANG_H_

#include "../kernel/ra_kernel.h"

#define RA__LANG_SUFFIX_U  0x01
#define RA__LANG_SUFFIX_L  0x02
#define RA__LANG_SUFFIX_LL 0x04
#define RA__LANG_SUFFIX_F  0x08

enum {
	RA__LANG_,
	RA__LANG_EOF,
	RA__LANG_INT,
	RA__LANG_REAL,
	RA__LANG_CHAR,
	RA__LANG_STRING,
	RA__LANG_IDENTIFIER,
	/*-*/
	RA__LANG_KEYWORD_,
	RA__LANG_KEYWORD_AUTO,
	RA__LANG_KEYWORD_BOOL,
	RA__LANG_KEYWORD_BREAK,
	RA__LANG_KEYWORD_CASE,
	RA__LANG_KEYWORD_CHAR,
	RA__LANG_KEYWORD_COMPLEX,
	RA__LANG_KEYWORD_CONST,
	RA__LANG_KEYWORD_CONTINUE,
	RA__LANG_KEYWORD_DEFAULT,
	RA__LANG_KEYWORD_DO,
	RA__LANG_KEYWORD_DOUBLE,
	RA__LANG_KEYWORD_ELSE,
	RA__LANG_KEYWORD_ENUM,
	RA__LANG_KEYWORD_EXTERN,
	RA__LANG_KEYWORD_FLOAT,
	RA__LANG_KEYWORD_FOR,
	RA__LANG_KEYWORD_GOTO,
	RA__LANG_KEYWORD_IF,
	RA__LANG_KEYWORD_IMAGINARY,
	RA__LANG_KEYWORD_INLINE,
	RA__LANG_KEYWORD_INT,
	RA__LANG_KEYWORD_LONG,
	RA__LANG_KEYWORD_REGISTER,
	RA__LANG_KEYWORD_RESTRICT,
	RA__LANG_KEYWORD_RETURN,
	RA__LANG_KEYWORD_SHORT,
	RA__LANG_KEYWORD_SIGNED,
	RA__LANG_KEYWORD_SIZEOF,
	RA__LANG_KEYWORD_STATIC,
	RA__LANG_KEYWORD_STRUCT,
	RA__LANG_KEYWORD_SWITCH,
	RA__LANG_KEYWORD_TYPEDEF,
	RA__LANG_KEYWORD_UNION,
	RA__LANG_KEYWORD_UNSIGNED,
	RA__LANG_KEYWORD_VOID,
	RA__LANG_KEYWORD_VOLATILE,
	RA__LANG_KEYWORD_WHILE,
	/*-*/
	RA__LANG_OPERATOR_,
	RA__LANG_OPERATOR_ADD,
	RA__LANG_OPERATOR_SUB,
	RA__LANG_OPERATOR_MUL,
	RA__LANG_OPERATOR_DIV,
	RA__LANG_OPERATOR_MOD,
	RA__LANG_OPERATOR_SHL,
	RA__LANG_OPERATOR_SHR,
	RA__LANG_OPERATOR_OR,
	RA__LANG_OPERATOR_XOR,
	RA__LANG_OPERATOR_AND,
	RA__LANG_OPERATOR_NOT,
	RA__LANG_OPERATOR_LOGIC_OR,
	RA__LANG_OPERATOR_LOGIC_AND,
	RA__LANG_OPERATOR_LOGIC_NOT,
	RA__LANG_OPERATOR_INC,
	RA__LANG_OPERATOR_DEC,
	RA__LANG_OPERATOR_LT,
	RA__LANG_OPERATOR_GT,
	RA__LANG_OPERATOR_LE,
	RA__LANG_OPERATOR_GE,
	RA__LANG_OPERATOR_EQ,
	RA__LANG_OPERATOR_NE,
	RA__LANG_OPERATOR_ASN,
	RA__LANG_OPERATOR_ADDASN,
	RA__LANG_OPERATOR_SUBASN,
	RA__LANG_OPERATOR_MULASN,
	RA__LANG_OPERATOR_DIVASN,
	RA__LANG_OPERATOR_MODASN,
	RA__LANG_OPERATOR_SHLASN,
	RA__LANG_OPERATOR_SHRASN,
	RA__LANG_OPERATOR_ORASN,
	RA__LANG_OPERATOR_XORASN,
	RA__LANG_OPERATOR_ANDASN,
	RA__LANG_OPERATOR_OPEN_BRACE,
	RA__LANG_OPERATOR_CLOSE_BRACE,
	RA__LANG_OPERATOR_OPEN_PARENTH,
	RA__LANG_OPERATOR_CLOSE_PARENTH,
	RA__LANG_OPERATOR_OPEN_BRACKET,
	RA__LANG_OPERATOR_CLOSE_BRACKET,
	RA__LANG_OPERATOR_DOT,
	RA__LANG_OPERATOR_COMMA,
	RA__LANG_OPERATOR_COLON,
	RA__LANG_OPERATOR_POINTER,
	RA__LANG_OPERATOR_QUESTION,
	RA__LANG_OPERATOR_SEMICOLON,
	RA__LANG_OPERATOR_DOTDOTDOT
};

enum {
	RA__LANG_NODE_,
	RA__LANG_NODE_EXPR_,
	RA__LANG_NODE_EXPR_LITERAL,    // u.[i,r,c,s]
	RA__LANG_NODE_EXPR_IDENTIFIER, // u.s
	RA__LANG_NODE_EXPR_REF,        // left . u.s
	RA__LANG_NODE_EXPR_PREF,       // left -> u.s
	RA__LANG_NODE_EXPR_CALL,       // left ( right )
	RA__LANG_NODE_EXPR_ARRAY,      // left [ right ]
	RA__LANG_NODE_EXPR_SIZEOF,     // sizeof ( right )
	RA__LANG_NODE_EXPR_CAST,       // ( left ) right
	RA__LANG_NODE_EXPR_NEG,        // - right
	RA__LANG_NODE_EXPR_NOT,        // ~ right
	RA__LANG_NODE_EXPR_LOGIC_NOT,  // ! right
	RA__LANG_NODE_EXPR_ADDRESS,    // & right
	RA__LANG_NODE_EXPR_DEREF,      // * right
	RA__LANG_NODE_EXPR_ADD,        // left +  right
	RA__LANG_NODE_EXPR_SUB,        // left -  right
	RA__LANG_NODE_EXPR_MUL,        // left *  right
	RA__LANG_NODE_EXPR_DIV,        // left /  right
	RA__LANG_NODE_EXPR_MOD,        // left %  right
	RA__LANG_NODE_EXPR_SHL,        // left << right
	RA__LANG_NODE_EXPR_SHR,        // left >> right
	RA__LANG_NODE_EXPR_OR,         // left |  right
	RA__LANG_NODE_EXPR_XOR,        // left ^  right
	RA__LANG_NODE_EXPR_AND,        // left &  right
	RA__LANG_NODE_EXPR_LOGIC_OR,   // left || right
	RA__LANG_NODE_EXPR_LOGIC_AND,  // left && right
	RA__LANG_NODE_EXPR_LT,         // left <  right
	RA__LANG_NODE_EXPR_GT,         // left >  right
	RA__LANG_NODE_EXPR_LE,         // left <= right
	RA__LANG_NODE_EXPR_GE,         // left >= right
	RA__LANG_NODE_EXPR_EQ,         // left == right
	RA__LANG_NODE_EXPR_NE,         // left != right
	RA__LANG_NODE_EXPR_INC,        // left ++ or ++ right
	RA__LANG_NODE_EXPR_DEC,        // left -- or -- right
	RA__LANG_NODE_EXPR_ASN,        // left   = right
	RA__LANG_NODE_EXPR_ADDASN,     // left  += right
	RA__LANG_NODE_EXPR_SUBASN,     // left  -= right
	RA__LANG_NODE_EXPR_MULASN,     // left  *= right
	RA__LANG_NODE_EXPR_DIVASN,     // left  /= right
	RA__LANG_NODE_EXPR_MODASN,     // left  %= right
	RA__LANG_NODE_EXPR_SHLASN,     // left <<= right
	RA__LANG_NODE_EXPR_SHRASN,     // left >>= right
	RA__LANG_NODE_EXPR_ORASN,      // left  |= right
	RA__LANG_NODE_EXPR_XORASN,     // left  ^= right
	RA__LANG_NODE_EXPR_ANDASN,     // left  &= right
	RA__LANG_NODE_EXPR_COND,       // cond ? left : right
	/*-*/
	RA__LANG_NODE_END
};

struct ra__lang_token {
	int op;
	int suffix;
	unsigned int lineno;
	unsigned int column;
	union {
		int c;         // RA__LANG_CHAR
		uint64_t i;    // RA__LANG_INT
		long double r; // RA__LANG_REAL
		const char *s; // RA__LANG_STRING
	} u;
};

struct ra__lang_node {
	int id;
	int op;
	struct ra__lang_node *link;
	struct ra__lang_node *cond;
	struct ra__lang_node *left;
	struct ra__lang_node *right;
	const struct ra__lang_token *token;
};

typedef struct ra__lang *ra__lang_t;

ra__lang_t ra__lang_open(const char *pathname);

void ra__lang_close(ra__lang_t lang);

struct ra__lang_node *ra__lang_root(ra__lang_t lang);

#endif // _RA_LANG_H_
