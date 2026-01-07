/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_lexer.h"

#define PAD    8
#define N_MAPS 199

#define TLEN ( sizeof (struct ra_lexer_token) )

#define ERROR(l, f, a)				\
	do {					\
		ra_printf(RA_COLOR_RED_BOLD,	\
			  "error: ");		\
		ra_printf(RA_COLOR_BLACK,	\
			  "%s:%u:%u: "		\
			  f "\n",		\
			  (l)->pathname,	\
			  (l)->lineno,		\
			  (l)->column,		\
			  (a));			\
		RA_TRACE("syntax error");	\
	}					\
	while (0)

struct ra_lexer {
	char *s;
	unsigned lineno;
	unsigned column;
	ra_vector_t tokens;
	const char *pathname;
	struct map {
		int op;
		const char *s;
	} maps[N_MAPS];
};

const char * const KEYWORDS[] = {
	"auto",    "double",   "int",      "struct",   "break",    "else",
	"long",    "switch",   "case",     "enum",     "register", "typedef",
	"char",    "extern",   "return",   "union",    "const",    "float",
	"short",   "unsigned", "continue", "for",      "signed",   "void",
	"default", "goto",     "sizeof",   "volatile", "do",       "if",
	"static",  "while"
};

const char * const OPERATORS[] = {
	"+",  "-",   "*",  "/",  "%",   "++", "--",  "==",  "!=", ">",  "<",
	">=", "<=",  "&&", "||", "!",   "&",  "|",   "^",   "~",  "<<", ">>",
	"=",  "+=",  "-=", "*=", "/=",  "%=", "<<=", ">>=", "&=", "^=", "|=",
	"&",  "*",   "?",  ":",  ".",   "->", "{",    "}",  "[",  "]",
	"(",  ")",  ",",   ";",  "...",	"`",  "@",   "#",   "$",  "\\"
};

static const char *
strdupl(const char *b, const char *e)
{
	size_t n;
	char *s;

	assert( b && (b < e) );

	n = e - b;
	if (!(s = malloc(n + 1))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memcpy(s, b, n);
	s[n] = '\0';
	return s;
}

static int
is_identifier(const char *b, const char *e)
{
	assert( b && (b < e) );

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

static void
populate(struct ra_lexer *lexer, const char *s, int op)
{
	uint64_t h, j;
	int i;

	assert( s && strlen(s) );
	assert( (RA_LEXER_KEYWORD_ < op) && (RA_LEXER_END > op) );

	h = ra_hash(s, strlen(s));
	for (i=0; i<N_MAPS; ++i) {
		j = (h + i) % N_MAPS;
		if (!lexer->maps[j].s) {
			lexer->maps[j].op = op;
			lexer->maps[j].s = s;
			return;
		}
	}
	RA_TRACE("software bug (abort)");
	abort();
}

static int
lookup(struct ra_lexer *lexer, const char *b, const char *e)
{
	uint64_t h, j;
	size_t n;
	int i;

	assert( b && (b < e) );

	n = e - b;
	h = ra_hash(b, n);
	for (i=0; i<N_MAPS; ++i) {
		j = (h + i) % N_MAPS;
		if (!lexer->maps[j].s) {
			break;
		}
		if ((n == strlen(lexer->maps[j].s) &&
		     !strncmp(b, lexer->maps[j].s, n))) {
			return lexer->maps[j].op;
		}
	}
	return 0;
}

static int
process(struct ra_lexer *lexer, const char *b, const char *e)
{
	struct ra_lexer_token *token;
	const char *s;
	int op;

	if (b < e) {
		if ((op = lookup(lexer, b, e))) {
			if (!(token = ra_vector_append(lexer->tokens, TLEN))) {
				RA_TRACE("^");
				return -1;
			}
			token->op = op;
			token->lineno = lexer->lineno;
			token->column = lexer->column - (unsigned)(e - b);
		}
		else if (is_identifier(b, e)) {
			if (!(s = strdupl(b, e)) ||
			    !(token = ra_vector_append(lexer->tokens, TLEN))) {
				RA_FREE(s);
				RA_TRACE("^");
				return -1;
			}
			token->op = RA_LEXER_IDENTIFIER;
			token->u.s = s;
			token->lineno = lexer->lineno;
			token->column = lexer->column - (unsigned)(e - b);
		}
		else {
			ERROR(lexer, "invalid token", "");
			return -1;
		}
	}
	return 0;
}

static char *
process_comment(struct ra_lexer *lexer, const char *b)
{
	const char *e;

	assert( ('/' == b[0]) && ('*' == b[1]) );

	e = b + 2;
	while (*e) {
		if ('\n' == (*e)) {
			lexer->lineno += 1;
			lexer->column  = 1;
			++e;
		}
		else if (('/' == e[0]) && ('*' == e[1])) {
			ERROR(lexer, "'/*' within block comment", "");
			return NULL;
		}
		else if (('*' == e[0]) && ('/' == e[1])) {
			lexer->column += 2;
			return (char *)(e + 2);
		}
		else {
			lexer->column += 1;
			++e;
		}
	}
	ERROR(lexer, "unterminated /* comment", "");
	return NULL;
}

static char *
process_string(struct ra_lexer *lexer, const char *b)
{
	struct ra_lexer_token *token;
	const char *s, *e;
	char quote;

	assert( ('\"' == b[0]) || ('\'' == b[0]) );

	e = b;
	quote = (*b);
	while ((*e) && ('\n' != (*e))) {
		lexer->column += 1;
		++e;
		if (quote == (*e)) {
			lexer->column += 1;
			++e;
			if (!(s = strdupl(b, e)) ||
			    !(token = ra_vector_append(lexer->tokens, TLEN))) {
				RA_FREE(s);
				RA_TRACE("^");
				return NULL;
			}
			token->op = RA_LEXER_STRING;
			token->u.s = s;
			token->lineno = lexer->lineno;
			token->column = lexer->column - (unsigned)(e - b);
			return (char *)e;
		}
		if ('\\' == (*e)) {
			lexer->column += 1;
			++e;
			if (!(*e)) {
				ERROR(lexer, "missing escape character", "");
				return NULL;
			}
		}
	}
	ERROR(lexer, "missing terminating '%c' character", quote);
	return NULL;
}

static char *
process_numeric(struct ra_lexer *lexer, const char *b)
{
	struct ra_lexer_token *token;
	const char *s, *e;

	e = b;
	while ((*e) && (('.' == (*e) || isdigit((unsigned char)(*e))))) {
		++e;
		++lexer->column;
	}
	if (!(s = strdupl(b, e)) ||
	    !(token = ra_vector_append(lexer->tokens, TLEN))) {
		RA_FREE(s);
		RA_TRACE("^");
		return NULL;
	}
	token->op = RA_LEXER_REAL;
	token->u.r = 1.2;
	token->lineno = lexer->lineno;
	token->column = lexer->column - (unsigned)(e - b);
	return (char *)e;
}

static int
tokenize(struct ra_lexer *lexer)
{
	struct ra_lexer_token *token;
	char *e, *b;
	int op;

	e = lexer->s;
	b = lexer->s;
	lexer->lineno = 1;
	lexer->column = 1;
	while (*e) {
		if (isspace((unsigned char)(*e))) {
			if (process(lexer, b, e)) {
				RA_TRACE("^");
				return -1;
			}
			if ('\n' == (*e)) {
				lexer->lineno += 1;
				lexer->column  = 1;
			}
			else {
				lexer->column += 1;
			}
			b = ++e;
		}
		else if (('/' == e[0]) && ('/' == e[1])) {
			if (process(lexer, b, e)) {
				RA_TRACE("^");
				return -1;
			}
			while ((*e) && ('\n' != (*e))) {
				++e;
			}
			b = e;
		}
		else if (('/' == e[0]) && ('*' == e[1])) {
			if (process(lexer, b, e) ||
			    !(e = process_comment(lexer, e))) {
				RA_TRACE("^");
				return -1;
			}
			b = e;
		}
		else if (('"' == (*e)) || ('\'' == (*e))) {
			if (process(lexer, b, e) ||
			    !(e = process_string(lexer, e))) {
				RA_TRACE("^");
				return -1;
			}
			b = e;
		}
		else if ((op = lookup(lexer, e, e + 3)) &&
			 (RA_LEXER_OPERATOR_ < op)) {
			if (process(lexer, b, e) ||
			    !(token = ra_vector_append(lexer->tokens, TLEN))) {
				RA_TRACE("^");
				return -1;
			}
			token->op = op;
			token->lineno = lexer->lineno;
			token->column = lexer->column;
			lexer->column += 3;
			e += 3;
			b = e;
		}
		else if ((op = lookup(lexer, e, e + 2)) &&
			 (RA_LEXER_OPERATOR_ < op)) {
			if (process(lexer, b, e) ||
			    !(token = ra_vector_append(lexer->tokens, TLEN))) {
				RA_TRACE("^");
				return -1;
			}
			token->op = op;
			token->lineno = lexer->lineno;
			token->column = lexer->column;
			lexer->column += 2;
			e += 2;
			b = e;
		}
		else if ((op = lookup(lexer, e, e + 1)) &&
			 (RA_LEXER_OPERATOR_ < op)) {
			if (process(lexer, b, e) ||
			    !(token = ra_vector_append(lexer->tokens, TLEN))) {
				RA_TRACE("^");
				return -1;
			}
			token->op = op;
			token->lineno = lexer->lineno;
			token->column = lexer->column;
			lexer->column += 1;
			e += 1;
			b = e;
		}
		else if ((b == e) &&
			 (isdigit((unsigned char)(*e)) ||
			  (('.' == e[0]) && isdigit((unsigned char)e[1])))) {
			if (!(e = process_numeric(lexer, e))) {
				RA_TRACE("^");
				return -1;
			}
			b = e;
		}
		else {
			lexer->column += 1;
			++e;
		}
	}
	if (process(lexer, b, e)) {
		RA_TRACE("^");
		return -1;
	}
	return 0;
}

ra_lexer_t
ra_lexer_open(const char *pathname)
{
	struct ra_lexer *lexer;
	const char *s;
	size_t i;

	assert( pathname && strlen(pathname) );

	if (!(lexer = malloc(sizeof (struct ra_lexer)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(lexer, 0, sizeof (struct ra_lexer));
	lexer->pathname = pathname;
	for (i=0; i<RA_ARRAY_SIZE(KEYWORDS); ++i) {
		populate(lexer, KEYWORDS[i], RA_LEXER_KEYWORD_ + (int)(i+1));
	}
	for (i=0; i<RA_ARRAY_SIZE(OPERATORS); ++i) {
		populate(lexer, OPERATORS[i], RA_LEXER_OPERATOR_ + (int)(i+1));
	}
	if (!(lexer->tokens = ra_vector_open())) {
		ra_lexer_close(lexer);
		RA_TRACE("^");
		return NULL;
	}
	if (!(s = ra_file_string_read(pathname))) {
		ra_lexer_close(lexer);
		RA_TRACE("^");
		return NULL;
	}
	if (!(lexer->s = malloc(strlen(s) + PAD))) {
		ra_lexer_close(lexer);
		RA_FREE(s);
		RA_TRACE("out of memory");
		return NULL;
	}
	memcpy(lexer->s, s, strlen(s));
	memset(lexer->s + strlen(s), 0, PAD);
	RA_FREE(s);
	if (tokenize(lexer)) {
		ra_lexer_close(lexer);
		RA_TRACE("^");
		return NULL;
	}
	return lexer;
}

void
ra_lexer_close(ra_lexer_t lexer)
{
	struct ra_lexer_token *token;
	uint64_t i;

	if (lexer) {
		if (lexer->tokens) {
			for (i=0; i<ra_vector_items(lexer->tokens); ++i) {
				token = ra_vector_lookup(lexer->tokens, i);
				if ((RA_LEXER_IDENTIFIER == token->op) ||
				    (RA_LEXER_STRING == token->op)) {
					RA_FREE(token->u.s);
				}
			}
			ra_vector_close(lexer->tokens);
		}
		RA_FREE(lexer->s);
		memset(lexer, 0, sizeof (struct ra_lexer));
		RA_FREE(lexer);
	}
}

const struct ra_lexer_token *
ra_lexer_lookup(ra_lexer_t lexer, uint64_t i)
{
	assert( lexer );

	return ra_vector_lookup(lexer->tokens, i);
}

uint64_t
ra_lexer_items(ra_lexer_t lexer)
{
	assert( lexer );

	return ra_vector_items(lexer->tokens);
}

const char *
ra_lexer_csv(ra_lexer_t lexer)
{
	const struct ra_lexer_token *token;
	const char *s, *pathname;
	char buf[256];
	FILE *file;
	uint64_t i;
	int op;

	assert( lexer );

	if (!(pathname = ra_pathname(NULL))) {
		RA_TRACE("^");
		return NULL;
	}
	if (!(file = fopen(pathname, "w"))) {
		RA_FREE(pathname);
		RA_TRACE("unable to open temporary file");
		return NULL;
	}
	if (0 > fprintf(file, "LINENO,COLUMN,OP,VALUE\n")) {
		fclose(file);
		ra_unlink(pathname);
		RA_FREE(pathname);
		RA_TRACE("file write failed");
		return NULL;
	}
	for (i=0; i<ra_lexer_items(lexer); ++i) {
		token = ra_lexer_lookup(lexer, i);
		if ((RA_LEXER_OPERATOR_ < token->op) &&
		    (RA_LEXER_END > token->op)) {
			op = token->op - RA_LEXER_OPERATOR_ - 1;
			ra_sprintf(buf,
				   sizeof (buf),
				   "%d,%d,\"%s\",",
				   token->lineno,
				   token->column,
				   OPERATORS[op]);
		}
		else if ((RA_LEXER_KEYWORD_ < token->op) &&
			 (RA_LEXER_OPERATOR_ > token->op)) {
			op = token->op - RA_LEXER_KEYWORD_ - 1;
			ra_sprintf(buf,
				   sizeof (buf),
				   "%d,%d,\"%s\",",
				   token->lineno,
				   token->column,
				   KEYWORDS[op]);
		}
		else if (RA_LEXER_INT == token->op) {
			ra_sprintf(buf,
				   sizeof (buf),
				   "%d,%d,INT,%ld",
				   token->lineno,
				   token->column,
				   token->u.i);
		}
		else if (RA_LEXER_REAL == token->op) {
			ra_sprintf(buf,
				   sizeof (buf),
				   "%d,%d,REAL,%Lf",
				   token->lineno,
				   token->column,
				   token->u.r);
		}
		else if (RA_LEXER_STRING == token->op) {
			ra_sprintf(buf,
				   sizeof (buf),
				   "%d,%d,STRING,[%s]",
				   token->lineno,
				   token->column,
				   token->u.s);
		}
		else if (RA_LEXER_IDENTIFIER == token->op) {
			ra_sprintf(buf,
				   sizeof (buf),
				   "%d,%d,IDENTIFIER,%s",
				   token->lineno,
				   token->column,
				   token->u.s);
		}
		else {
			assert( 0 );
		}
		if (0 > fprintf(file, "%s\n", buf)) {
			fclose(file);
			ra_unlink(pathname);
			RA_FREE(pathname);
			RA_TRACE("file write failed");
			return NULL;
		}
	}
	if (fclose(file)) {
		ra_unlink(pathname);
		RA_FREE(pathname);
		RA_TRACE("file write failed");
		return NULL;
	}
	if (!(s = ra_file_string_read(pathname))) {
		ra_unlink(pathname);
		RA_FREE(pathname);
		RA_TRACE("^");
		return NULL;
	}
	ra_unlink(pathname);
	RA_FREE(pathname);
	return s;
}
