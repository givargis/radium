/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_lang_lexer.c
 */

#include "../utils/ra_utils.h"
#include "ra_lang_lexer.h"

#define N_MAPS 87

#define TRACE(l,m,x)					\
	do {						\
		ra__log("error: %s:%u:%u: " m,		\
			(l)->pathname,			\
			(l)->lineno,			\
			(l)->column,			\
			(x));				\
		RA__ERROR_TRACE(RA__ERROR_SYNTAX);	\
	}						\
	while (0)

struct ra__lang_lexer {
	char *s;
	unsigned int lineno;
	unsigned int column;
	const char *pathname;
	struct {
		int op;
		const char *key;
	} *maps;
	struct {
		uint64_t size;
		uint64_t capacity;
		struct ra__lang_token *tokens;
	} tokens;
};

const char * const KEYWORDS[] = {
	"auto",     "_Bool",   "break",    "case",       "char",
	"_Complex", "const",   "continue", "default",    "do",
	"double",   "else",    "enum",     "extern",     "float",
	"for",      "goto",    "if",       "_Imaginary", "inline",
	"int",      "long",    "register", "restrict",   "return",
	"short",    "signed",  "sizeof",   "static",     "struct",
	"switch",   "typedef", "union",    "unsigned",   "void",
	"volatile", "while"
};

const char * const OPERATORS[] = {
	"+",   "-",   "*",   "/",  "%",  "<<", ">>", "|",  "^",
	"&",   "~",   "||",  "&&", "!",  "++", "--", "<",  ">",
	"<=",  ">=",  "==",  "!=", "=",  "+=", "-=", "*=", "/=",
	"%=",  "<<=", ">>=", "|=", "^=", "&=", "{",  "}",  "(",
	")",   "[",   "]",   ".",  ",",  ":",  "->", "?",  ";",
	"..."
};

static void
map_insert(struct ra__lang_lexer *lexer, const char *key, int op)
{
	uint64_t j;

	j = ra__hash(key, ra__strlen(key));
	for (int i=0; i<N_MAPS; ++i) {
		j = (j + 1) % N_MAPS;
		if (!lexer->maps[j].key) { // insert
			lexer->maps[j].key = key;
			lexer->maps[j].op = op;
			return;
		}
		if (!strcmp(lexer->maps[j].key, key)) { // update
			break;
		}
	}
	RA__ERROR_HALT(RA__ERROR_SOFTWARE);
}

static int
map_lookup(const struct ra__lang_lexer *lexer, const char *b, const char *e)
{
	uint64_t j, n;

	n = e - b;
	j = ra__hash(b, n);
	for (int i=0; i<N_MAPS; ++i) {
		j = (j + 1) % N_MAPS;
		if (!lexer->maps[j].key) {
			break;
		}
		if ((n == ra__strlen(lexer->maps[j].key)) &&
		    !strncmp(lexer->maps[j].key, b, n)) {
			return lexer->maps[j].op;
		}
	}
	return 0;
}

static int
map_configure(struct ra__lang_lexer *lexer)
{
	if (!(lexer->maps = ra__malloc(N_MAPS * sizeof (lexer->maps[0])))) {
		RA__ERROR_TRACE(0);
		return -1;
	}
	memset(lexer->maps, 0, N_MAPS * sizeof (lexer->maps[0]));
	for (int i=0; i<(int)RA__ARRAY_SIZE(KEYWORDS); ++i) {
		map_insert(lexer, KEYWORDS[i], RA__LANG_KEYWORD_ + i + 1);
	}
	for (int i=0; i<(int)RA__ARRAY_SIZE(OPERATORS); ++i) {
		map_insert(lexer, OPERATORS[i], RA__LANG_OPERATOR_ + i + 1);
	}
	map_insert(lexer, "<%", RA__LANG_OPERATOR_OPEN_BRACE);
	map_insert(lexer, "%>", RA__LANG_OPERATOR_CLOSE_BRACE);
	map_insert(lexer, "<:", RA__LANG_OPERATOR_OPEN_BRACKET);
	map_insert(lexer, ":>", RA__LANG_OPERATOR_CLOSE_BRACKET);
	return 0;
}

static int
map_keyword(struct ra__lang_lexer *lexer, const char *b, const char *e)
{
	int op;

	if ((op = map_lookup(lexer, b, e))) {
		if (RA__LANG_OPERATOR_ > op) {
			return op;
		}
	}
	return 0;
}

static int
map_operator(const struct ra__lang_lexer *lexer, const char *b, const char **e)
{
	int op;

	if ((op = map_lookup(lexer, b, b + 3))) {
		if (RA__LANG_OPERATOR_ < op) {
			(*e) = b + 3;
			return op;
		}
	}
	if ((op = map_lookup(lexer, b, b + 2))) {
		if (RA__LANG_OPERATOR_ < op) {
			(*e) = b + 2;
			return op;
		}
	}
	if ((op = map_lookup(lexer, b, b + 1))) {
		if (RA__LANG_OPERATOR_ < op) {
			(*e) = b + 1;
			return op;
		}
	}
	return 0;
}

static char *
strdupl(const char *b, const char *e)
{
	uint64_t n;
	char *s;

	n = e - b;
	if (!(s = ra__malloc(n + 1))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memcpy(s, b, n);
	s[n] = '\0';
	return s;
}

static struct ra__lang_token *
mktoken(struct ra__lang_lexer *lexer, int op, uint64_t width)
{
	struct ra__lang_token *token, *tokens;
	uint64_t n, capacity;

	if (lexer->tokens.size >= lexer->tokens.capacity) {
		capacity = lexer->tokens.size + 1024;
		n = capacity * sizeof (tokens[0]);
		if (!(tokens = ra__realloc(lexer->tokens.tokens, n))) {
			RA__ERROR_TRACE(0);
			return NULL;
		}
		lexer->tokens.tokens = tokens;
		lexer->tokens.capacity = capacity;
	}
	token = &lexer->tokens.tokens[lexer->tokens.size++];
	memset(token, 0, sizeof (tokens[0]));
	token->op = op;
	token->lineno = lexer->lineno;
	token->column = lexer->column - (unsigned)width;
	return token;
}

static int
is_identifier(const char *b, const char *e)
{
	if (('_' == (*b)) || isalpha((unsigned char)(*b))) {
		while (++b < e) {
			if (('_' != (*b)) && !isalnum((unsigned char)(*b))) {
				return 0;
			}
		}
		return 1;
	}
	return 0;
}

static int
hexval(int c)
{
	c = tolower(c);
	if (isdigit(c)) {
		return c - '0';
	}
	if (('a' <= c) && ('f' >= c)) {
		return c - 'a' + 10;
	}
	return -1;
}

static int
charval(const char *s, int *c)
{
	int v;

	(*c) = -1;
	if ('\\' == (*s)) {
		switch (*(++s)) {
		case 'a' : (*c) = '\a'; ++s; break;
		case 'b' : (*c) = '\b'; ++s; break;
		case 'f' : (*c) = '\f'; ++s; break;
		case 'n' : (*c) = '\n'; ++s; break;
		case 'r' : (*c) = '\r'; ++s; break;
		case 't' : (*c) = '\t'; ++s; break;
		case 'v' : (*c) = '\v'; ++s; break;
		case '\\': (*c) = '\\'; ++s; break;
		case '\'': (*c) = '\''; ++s; break;
		case '\"': (*c) = '\"'; ++s; break;
		case '\?': (*c) = '\?'; ++s; break;
		case 'x':
			++s;
			(*c) = 0;
			for (int i=0; i<8; ++i,++s) {
				if (0 > (v = hexval((unsigned char)*s))) {
					break;
				}
				(*c) = (*c) * 16 + v;
			}
			break;
		default:
			(*c) = 0;
			for (int i=0; i<3; ++i,++s) {
				if (('0' > (*s)) || ('7' < (*s))) {
					break;
				}
				(*c) = (*c) * 8 + (int)(*s) - '0';
			}
		}
	}
	else if ('\0' != (*s)) {
		(*c) = (int)(*(s++));
	}
	return ((0 > (*c)) || ('\0' != (*s))) ? -1 : 0;
}

static const char *
eat_eol(char *s)
{
	while (*s) {
		if ('\n' == (*s)) {
			++s;
			break;
		}
		(*s) = ' ';
		++s;
	}
	return s;
}

static char *
eat_suffix(char *s, int *suffix, int mask)
{
	(*suffix) = 0;
	while (*s) {
		if (('u' == (*s)) || ('U' == (*s))) {
			if (RA__LANG_SUFFIX_U & (*suffix)) {
				return NULL;
			}
			(*suffix) |= RA__LANG_SUFFIX_U;
		}
		else if (('l' == (*s)) || ('L' == (*s))) {
			if (RA__LANG_SUFFIX_LL & (*suffix)) {
				return NULL;
			}
			if (RA__LANG_SUFFIX_L & (*suffix)) {
				(*suffix) &= ~RA__LANG_SUFFIX_L;
				(*suffix) |= RA__LANG_SUFFIX_LL;
			}
			else {
				(*suffix) |= RA__LANG_SUFFIX_L;
			}
		}
		else if (('f' == (*s)) || ('F' == (*s))) {
			if (RA__LANG_SUFFIX_F & (*suffix)) {
				return NULL;
			}
			(*suffix) |= RA__LANG_SUFFIX_F;
		}
		else {
			break;
		}
		s++;
	}
	return ((*suffix) & ~mask) ? NULL : s;
}

static const char *
eat_comment(struct ra__lang_lexer *lexer, char *s)
{
	s[0] = s[1] = ' ';
	while (*s) {
		if ('\n' == (*s)) {
			lexer->lineno += 1;
			lexer->column  = 0;
		}
		else if (('/' == s[0]) && ('*' == s[1])) {
			TRACE(lexer, "'/*' within block comment", "");
			return NULL;
		}
		else if (('*' == s[0]) && ('/' == s[1])) {
			s[0] = s[1] = ' ';
			lexer->column += 2;
			return s + 2;
		}
		else {
			(*s) = ' ';
		}
		++lexer->column;
		++s;
	}
	TRACE(lexer, "unterminated comment", "");
	return NULL;
}

static int
eat_comments(struct ra__lang_lexer *lexer)
{
	const char *s;
	int skip;

	skip = 0;
	s = lexer->s;
	lexer->lineno = 1;
	lexer->column = 1;
	while (*s) {
		if (!skip) {
			if ('"' == (*s)) {
				skip = 1;
			}
			if ('\'' == (*s)) {
				skip = 2;
			}
		}
		else if ((1 == skip) && ('"' == (*s))) {
			skip = 0;
		}
		else if ((2 == skip) && ('\'' == (*s))) {
			skip = 0;
		}
		/*-*/
		if (!skip && ('/' == s[0]) && ('/' == s[1])) {
			s = eat_eol((char *)s);
			lexer->lineno += 1;
			lexer->column  = 1;
		}
		else if (!skip && ('/' == s[0]) && ('*' == s[1])) {
			if (!(s = eat_comment(lexer, (char *)s))) {
				RA__ERROR_TRACE(0);
				return -1;
			}
		}
		else if ('\n' == (*s)) {
			lexer->lineno += 1;
			lexer->column  = 1;
			++s;
		}
		else {
			++lexer->column;
			++s;
		}
	}
	return 0;
}

static const char *
eat_string(struct ra__lang_lexer *lexer, const char *s, char delim)
{
	while (*s) {
		if (delim == (*s)) {
			++lexer->column;
			++s;
			break;
		}
		else if ('\\' == (*s)) {
			++lexer->column;
			++s;
		}
		else if ('\n' == (*s)) {
			TRACE(lexer, "missing terminating character", "");
			return NULL;
		}
		++lexer->column;
		++s;
	}
	return s;
}

static int
process_string(struct ra__lang_lexer *lexer, const char *b, const char *e)
{
	struct ra__lang_token *token;
	const char *s;

	if (!(s = strdupl(b + 1, e - 1))) {
		RA__ERROR_TRACE(0);
		return -1;
	}
	if ('\'' == (*b)) {
		if (!(token = mktoken(lexer, RA__LANG_CHAR, e - b))) {
			RA__ERROR_TRACE(0);
			return -1;
		}
		if (charval(s, &token->u.c)) {
			TRACE(lexer, "invalid character literal '%s'", s);
			RA__FREE(s);
			return -1;
		}
		RA__FREE(s);
	}
	else {
		if (!(token = mktoken(lexer, RA__LANG_STRING, e - b))) {
			RA__ERROR_TRACE(0);
			return -1;
		}
		token->u.s = s;
	}
	return 0;
}

static const char *
process_numeric(struct ra__lang_lexer *lexer, const char *s)
{
	struct ra__lang_token *token;
	char *e, *er, *eh, *ed;
	uint64_t ih, id;
	ra__real_t r;
	int suffix;

	// real

	errno = 0;
	r = (ra__real_t)strtold(s, &er);
	if ((EINVAL == errno) || (ERANGE == errno)) {
		TRACE(lexer, "invalid real value", "");
		return NULL;
	}

	// int (hex)

	errno = 0;
	ih = (uint64_t)strtoull(s, &eh, 16);
	if ((EINVAL == errno) || (ERANGE == errno)) {
		TRACE(lexer, "invalid integer value", "");
		return NULL;
	}

	// int (dec/oct)

	errno = 0;
	id = (uint64_t)strtoull(s, &ed, 16);
	if ((EINVAL == errno) || (ERANGE == errno)) {
		TRACE(lexer, "invalid integer value", "");
		return NULL;
	}

	// which?

	if ((er > eh) && (er > ed)) {
		if (!(e = eat_suffix(er,
				     &suffix,
				     RA__LANG_SUFFIX_F |
				     RA__LANG_SUFFIX_L))) {
			TRACE(lexer, "invalid real suffix", "");
			RA__ERROR_TRACE(0);
			return NULL;
		}
		if (!(token = mktoken(lexer, RA__LANG_REAL, 0))) {
			RA__ERROR_TRACE(0);
			return NULL;
		}
		token->u.r = r;
		token->suffix = suffix;
		if (('.' == (*e)) || isalnum((unsigned char)(*e))) {
			TRACE(lexer, "invalid integer", "");
			return NULL;
		}
		return e;
	}
	if (eh > ed) {
		if (!(e = eat_suffix(eh,
				     &suffix,
				     RA__LANG_SUFFIX_U |
				     RA__LANG_SUFFIX_L |
				     RA__LANG_SUFFIX_LL))) {
			TRACE(lexer, "invalid integer suffix", "");
			return NULL;
		}
		if (!(token = mktoken(lexer, RA__LANG_INT, 0))) {
			RA__ERROR_TRACE(0);
			return NULL;
		}
		token->u.i = ih;
		token->suffix = suffix;
		if (('.' == (*e)) || isalnum((unsigned char)(*e))) {
			TRACE(lexer, "invalid integer", "");
			return NULL;
		}
		return e;
	}
	if (1) {
		if (!(e = eat_suffix(ed,
				     &suffix,
				     RA__LANG_SUFFIX_U |
				     RA__LANG_SUFFIX_L |
				     RA__LANG_SUFFIX_LL))) {
			TRACE(lexer, "invalid integer suffix", "");
			return NULL;
		}
		if (!(token = mktoken(lexer, RA__LANG_INT, 0))) {
			RA__ERROR_TRACE(0);
			return NULL;
		}
		token->u.i = id;
		token->suffix = suffix;
		if (('.' == (*e)) || isalnum((unsigned char)(*e))) {
			TRACE(lexer, "invalid integer", "");
			return NULL;
		}
		return e;
	}
	return NULL; // impossible
}

static int
process(struct ra__lang_lexer *lexer, const char *b, const char *e)
{
	struct ra__lang_token *token;
	int op;

	if (b < e) {
		if ((op = map_keyword(lexer, b, e))) {
			if (!(token = mktoken(lexer, op, e - b))) {
				RA__ERROR_TRACE(0);
				return -1;
			}
		}
		else if (is_identifier(b, e)) {
			if (!(token = mktoken(lexer,
					      RA__LANG_IDENTIFIER,
					      e - b)) ||
			    !(token->u.s = strdupl(b, e))) {
				RA__ERROR_TRACE(0);
				return -1;
			}
		}
		else {
			TRACE(lexer, "unrecognized character", "");
			return -1;
		}
	}
	return 0;
}

static const char *
operator(struct ra__lang_lexer *lexer, const char *p, const char *s)
{
	const char *e;
	int op;

	if ((op = map_operator(lexer, s, &e))) {
		if (process(lexer, p, s) || !mktoken(lexer, op, 0)) {
			RA__ERROR_TRACE(0);
			return NULL;
		}
		lexer->column += e - s;
		return e;
	}
	return s;
}

static int
tokenize(struct ra__lang_lexer *lexer)
{
	const char *s, *p, *r;

	s = lexer->s;
	p = lexer->s;
	lexer->lineno = 1;
	lexer->column = 1;
	while (*s) {
		if (('"' == s[0]) || ('\'' == s[0])) {
			if (process(lexer, p, s)) {
				RA__ERROR_TRACE(0);
				return -1;
			}
			p = s;
			++lexer->column;
			if (!(s = eat_string(lexer, s + 1, (*s)))) {
				RA__ERROR_TRACE(0);
				return -1;
			}
			if (process_string(lexer, p, s)) {
				RA__ERROR_TRACE(0);
				return -1;
			}
			++lexer->column;
			p = s;
		}
		else if ((p == s) &&
			 ((('.' == s[0]) && isdigit((unsigned char)s[1])) ||
			  isdigit((unsigned char)(*s)))) {
			p = s;
			if (!(s = process_numeric(lexer, p))) {
				RA__ERROR_TRACE(0);
				return -1;
			}
			lexer->column += s - p;
			p = s;
		}
		else if (isspace((unsigned char)(*s))) {
			if (process(lexer, p, s)) {
				RA__ERROR_TRACE(0);
				return -1;
			}
			if ('\n' == (*s)) {
				lexer->lineno += 1;
				lexer->column  = 0;
			}
			++lexer->column;
			++s;
			p = s;
		}
		else {
			if (!(r = operator(lexer, p, s))) {
				RA__ERROR_TRACE(0);
				return -1;
			}
			if (s == r) {
				++lexer->column;
				++s;
			}
			else {
				s = r;
				p = s;
			}
		}
	}
	if (process(lexer, p, s) || !mktoken(lexer, RA__LANG_EOF, 0)) {
		RA__ERROR_TRACE(0);
		return -1;
	}
	return 0;
}

ra__lang_lexer_t
ra__lang_lexer_open(const char *pathname)
{
	struct ra__lang_lexer *lexer;

	assert( ra__strlen(pathname) );

	if (!(lexer = ra__malloc(sizeof (struct ra__lang_lexer)))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(lexer, 0, sizeof (struct ra__lang_lexer));
	if (!(lexer->pathname = ra__strdup(pathname)) || map_configure(lexer)) {
		ra__lang_lexer_close(lexer);
		RA__ERROR_TRACE(0);
		return NULL;
	}
	if (!(lexer->s = (char *)ra__file_read_content(lexer->pathname))) {
		ra__log("error: unable to open '%s'", lexer->pathname);
		ra__lang_lexer_close(lexer);
		RA__ERROR_TRACE(0);
		return NULL;
	}
	if (eat_comments(lexer) || tokenize(lexer)) {
		ra__lang_lexer_close(lexer);
		RA__ERROR_TRACE(0);
		return NULL;
	}
	return lexer;
}

void
ra__lang_lexer_close(ra__lang_lexer_t lexer)
{
	struct ra__lang_token *token;

	if (lexer) {
		if (lexer->tokens.tokens) {
			for (uint64_t i=0; i<lexer->tokens.size; ++i) {
				token = &lexer->tokens.tokens[i];
				if ((RA__LANG_STRING == token->op) ||
				    (RA__LANG_IDENTIFIER == token->op)) {
					RA__FREE(token->u.s);
				}
			}
		}
		RA__FREE(lexer->tokens.tokens);
		RA__FREE(lexer->pathname);
		RA__FREE(lexer->maps);
		RA__FREE(lexer->s);
		memset(lexer, 0, sizeof (struct ra__lang_lexer));
	}
	RA__FREE(lexer);
}

const struct ra__lang_token *
ra__lang_lexer_lookup(ra__lang_lexer_t lexer, uint64_t i)
{
	assert( lexer );
	assert( i < lexer->tokens.size );

	return &lexer->tokens.tokens[i];
}

uint64_t
ra__lang_lexer_size(ra__lang_lexer_t lexer)
{
	assert( lexer );

	return lexer->tokens.size;
}
