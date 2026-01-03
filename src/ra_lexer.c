/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_lexer.h"

#define PAD    8
#define N_MAPS 160 /* ~ 2 x items */

#define ERROR(l,m)				\
	do {					\
		ra_printf(RA_COLOR_RED_BOLD,	\
			  "error: ");		\
		ra_printf(RA_COLOR_BLACK,	\
			  "lexer: "		\
			  "%u:%u: %s\n",	\
			  (l)->lineno,		\
			  (l)->column,		\
			  (m));			\
		RA_TRACE("syntax error");	\
	}					\
	while (0)

struct ra_lexer {
	char *s;
	unsigned lineno;
	unsigned column;
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
	"+",  "-",   "*",  "/",  "%",  "++", "--",  "==",  "!=", ">",  "<",
	">=", "<=",  "&&", "||", "!",  "&",  "|",   "^",   "~",  "<<", ">>",
	"=",  "+=",  "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=",
	"&",  "*",   "?",  ":",  ".",  "->", "[",   "]",   "(",  ")",  ",",
	";",  "..."
};

static const char *
strdupl(const char *b, const char *e)
{
	size_t n;
	char *s;

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
	uint64_t i, j, h;

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
lookup(ra_lexer_t lexer, const char *b, const char *e)
{
	uint64_t i, j, h;
	size_t n;

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

static void
unixclone(struct ra_lexer *lexer, const char *s)
{
	char *p;

	p = lexer->s;
	while (*s) {
		if (('\r' == s[0]) && ('\n' == s[1])) {
			(*(p++)) = '\n';
			s += 2;
		} else if ('\r' == (*s)) {
			(*(p++)) = '\n';
			s += 1;
		} else {
			(*(p++)) = (*(s++));
		}
	}
	memset(p, 0, PAD);
}

static int
process(struct ra_lexer *lexer, const char *b, const char *e)
{
	const char *s;
	int op;

	if (b < e) {
		if ((op = lookup(lexer, b, e))) {
			printf("KEYWORD: %d %u %u\n",
			       op,
			       lexer->lineno,
			       lexer->column - (unsigned)(e - b));
		} else if (is_identifier(b, e)) {
			if (!(s = strdupl(b, e))) {
				RA_TRACE("^");
				return -1;
			}
			printf("IDENTIFIER: [%s] %u %u\n",
			       s,
			       lexer->lineno,
			       lexer->column - (unsigned)strlen(s));
			RA_FREE(s);
		} else {
			ERROR(lexer, "invalid identifier");
			return -1;
		}
	}
	return 0;
}

static char *
process_string(struct ra_lexer *lexer, const char *b)
{
	const char *s, *e;

	e = b;
	while ((*e) && ('\n' != (*e))) {
		lexer->column += 1;
		++e;
		if ((*b) == (*e)) {
			lexer->column += 1;
			++e;
			if (!(s = strdupl(b, e))) {
				RA_TRACE("^");
				return NULL;
			}
			printf("STRING: [%s] %u %u\n",
			       s,
			       lexer->lineno,
			       lexer->column - (unsigned)strlen(s));
			RA_FREE(s);
			return (char *)e;
		}
		if ('\\' == (*e)) {
			lexer->column += 1;
			++e;
		}
		lexer->column += 1;
		++e;
	}
	ERROR(lexer, "missing terminating character");
	return NULL;
}

static char *
process_numeric(struct ra_lexer *lexer, const char *b)
{
	const char *s, *e;

	e = b;
	while ((*e) && (('.' == (*e) || isdigit((unsigned char)(*e))))) {
		++e;
		++lexer->column;
	}
	if (!(s = strdupl(b, e))) {
		RA_TRACE("^");
		return NULL;
	}
	printf("NUMERIC: [%s] %u %u\n",
	       s,
	       lexer->lineno,
	       lexer->column - (unsigned)strlen(s));
	RA_FREE(s);
	return (char *)e;
}

static int
tokenize(struct ra_lexer *lexer)
{
	char *e, *b;
	int op;

	e = lexer->s;
	b = lexer->s;
	lexer->lineno = 1;
	lexer->column = 1;
	while (*e) {
		if ('\n' == (*e)) {
			lexer->lineno += 1;
			lexer->column  = 1;
		} else if (isspace((unsigned char)(*e))) {
			if (process(lexer, b, e)) {
				RA_TRACE("^");
				return -1;
			}
			lexer->column += 1;
			++e;
			b = e;
		} else if ((b == e) && (('"' == (*e)) || ('\'' == (*e)))) {
			if (!(e = process_string(lexer, e))) {
				RA_TRACE("^");
				return -1;
			}
			b = e;
		} else if ((b == e) &&
			   (isdigit((unsigned char)(*e)) ||
			    (('.' == e[0]) && isdigit((unsigned char)e[1])))) {
			if (!(e = process_numeric(lexer, e))) {
				RA_TRACE("^");
				return -1;
			}
			b = e;
		} else if ((op = lookup(lexer, e, e + 3)) &&
			   (RA_LEXER_OPERATOR_ < op)) {
			if (process(lexer, b, e)) {
				RA_TRACE("^");
				return -1;
			}
			printf("OPERATOR: %d %u %u\n",
			       op,
			       lexer->lineno,
			       lexer->column);
			lexer->column += 3;
			e += 3;
			b = e;
		} else if ((op = lookup(lexer, e, e + 2)) &&
			   (RA_LEXER_OPERATOR_ < op)) {
			if (process(lexer, b, e)) {
				RA_TRACE("^");
				return -1;
			}
			printf("OPERATOR: %d %u %u\n",
			       op,
			       lexer->lineno,
			       lexer->column);
			lexer->column += 2;
			e += 2;
			b = e;
		} else if ((op = lookup(lexer, e, e + 1)) &&
			   (RA_LEXER_OPERATOR_ < op)) {
			if (process(lexer, b, e)) {
				RA_TRACE("^");
				return -1;
			}
			printf("OPERATOR: %d %u %u\n",
			       op,
			       lexer->lineno,
			       lexer->column);
			lexer->column += 1;
			e += 1;
			b = e;
		} else {
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
ra_lexer_open(const char *s)
{
	struct ra_lexer *lexer;
	size_t i;

	assert( s );

	if (!(lexer = malloc(sizeof (struct ra_lexer)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(lexer, 0, sizeof (struct ra_lexer));
	if (!(lexer->s = malloc(strlen(s) + PAD))) {
		ra_lexer_close(lexer);
		RA_TRACE("out of memory");
		return NULL;
	}
	for (i=0; i<RA_ARRAY_SIZE(KEYWORDS); ++i) {
		populate(lexer, KEYWORDS[i], RA_LEXER_KEYWORD_ + (int)(i+1));
	}
	for (i=0; i<RA_ARRAY_SIZE(OPERATORS); ++i) {
		populate(lexer, OPERATORS[i], RA_LEXER_OPERATOR_ + (int)(i+1));
	}
	unixclone(lexer, s);
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
	if (lexer) {
		RA_FREE(lexer->s);
		memset(lexer, 0, sizeof (struct ra_lexer));
		RA_FREE(lexer);
	}
}
