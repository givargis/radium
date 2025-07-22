//
// Copyright (c) Tony Givargis, 2024-2025
//
// ra_json.c
//

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include "ra_kernel.h"
#include "ra_json.h"

#define TRACE(j, m)				\
	do {					\
		(j)->curr = "";			\
		(j)->token.op = OP_END;		\
		ra_log("info: json:%u:%s",	\
		       (j)->lineno,		\
		       (m));			\
		RA_TRACE("syntax");		\
	}					\
	while (0)

#define MKN(j, n, t)				\
	do {					\
		if (!((n) = allocate((j)))) {	\
			RA_TRACE(NULL);		\
			return NULL;		\
		}				\
		(n)->op = (t);			\
	}					\
	while (0)

struct ra_json {
	char *curr;
	char *content;
	unsigned lineno;
	struct ra_json_node *root;
	struct {
		enum {
			OP_END,
			OP_NULL,
			OP_BOOL,
			OP_COMMA,
			OP_COLON,
			OP_STRING,
			OP_NUMBER,
			OP_OPEN_BRACE,
			OP_CLOSE_BRACE,
			OP_OPEN_BRACKET,
			OP_CLOSE_BRACKET
		} op;
		union {
			int bool;
			double number;
			const char *string;
		} u;
	} token;
	/*-*/
	void *chunk;
	size_t size;
};

static struct ra_json_node *
allocate(struct ra_json *json)
{
	const size_t CHUNK_SIZE = 4096;
	struct ra_json_node *node;
	size_t size;
	void *chunk;

	size = sizeof (struct ra_json_node);
	if (!json->chunk || (CHUNK_SIZE < (json->size + size))) {
		if (!(chunk = malloc(CHUNK_SIZE))) {
			RA_TRACE("out of memory");
			return NULL;
		}
		(*((void **)chunk)) = json->chunk;
		json->size = sizeof (struct ra_json_node *);
		json->chunk = chunk;
	}
	node = (struct ra_json_node *)((char *)json->chunk + json->size);
	json->size += size;
	memset(node, 0, size);
	return node;
}

static int
ishex(char c)
{
	c = tolower(c);
	if (isdigit((unsigned char)c)) {
		return 1;
	}
	if (('a' <= c) && ('f' >= c)) {
		return 1;
	}
	return 0;
}

static char *
eat_string(char *s)
{
	while (*++s) {
		if ('\"' == (*s)) {
			return s;
		}
		else if ('\\' == (*s)) {
			++s;
			if (('"' == (*s)) ||
			    ('\\' == (*s)) ||
			    ('/' == (*s)) ||
			    ('b' == (*s)) ||
			    ('f' == (*s)) ||
			    ('n' == (*s)) ||
			    ('r' == (*s)) ||
			    ('t' == (*s))) {
				++s;
				continue;
			}
			else if ('u' == (*s)) {
				for (int i=0; i<4; ++i) {
					if (!ishex(*++s)) {
						return NULL;
					}
				}
				continue;
			}
			else {
				return NULL;
			}
		}
		else if (('\r' == (*s)) || ('\n' == (*s))) {
			break;
		}
	}
	return NULL;
}

static void
forward(struct ra_json *json)
{
	char *s, *e;

	s = json->curr;
	while (*s) {
		if (isspace((unsigned char)(*s))) {
			if ('\n' == (*s)) {
				++json->lineno;
			}
			++s;
			continue;
		}
		if ('{' == (*s)) {
			json->curr = s + 1;
			json->token.op = OP_OPEN_BRACE;
			return;
		}
		if ('}' == (*s)) {
			json->curr = s + 1;
			json->token.op = OP_CLOSE_BRACE;
			return;
		}
		if ('[' == (*s)) {
			json->curr = s + 1;
			json->token.op = OP_OPEN_BRACKET;
			return;
		}
		if (']' == (*s)) {
			json->curr = s + 1;
			json->token.op = OP_CLOSE_BRACKET;
			return;
		}
		if (':' == (*s)) {
			json->curr = s + 1;
			json->token.op = OP_COLON;
			return;
		}
		if (',' == (*s)) {
			json->curr = s + 1;
			json->token.op = OP_COMMA;
			return;
		}
		if (!strncmp("null", s, 4)) {
			json->curr = s + 4;
			json->token.op = OP_NULL;
			return;
		}
		if (!strncmp("true", s, 4)) {
			json->curr = s + 4;
			json->token.op = OP_BOOL;
			json->token.u.bool = 1;
			return;
		}
		if (!strncmp("false", s, 5)) {
			json->curr = s + 5;
			json->token.op = OP_BOOL;
			json->token.u.bool = 0;
			return;
		}
		if ('\"' == (*s)) {
			if (!(e = eat_string(s))) {
				TRACE(json, "erroneous string");
				return;
			}
			(*e) = '\0';
			json->curr = e + 1;
			json->token.op = OP_STRING;
			json->token.u.string = s + 1;
			return;
		}
		else if (('-' == (*s)) ||
			 ('.' == (*s)) ||
			 isdigit((unsigned char)(*s))) {
			errno = 0;
			double number = strtod(s, &e);
			if ((EINVAL == errno) || (ERANGE == errno)) {
				TRACE(json, "erroneous number");
				return;
			}
			json->curr = e;
			json->token.op = OP_NUMBER;
			json->token.u.number = number;
			return;
		}
		TRACE(json, "erroneous character");
		return;
	}
	json->token.op = OP_END;
}

static struct ra_json_node *jarray(struct ra_json *json);
static struct ra_json_node *jobject(struct ra_json *json);

static struct ra_json_node *
jvalue(struct ra_json *json)
{
	struct ra_json_node *node;

	node = NULL;
	if (OP_NULL == json->token.op) {
		MKN(json, node, RA_JSON_NODE_OP_NULL);
		forward(json);
	}
	else if (OP_BOOL == json->token.op) {
		MKN(json, node, RA_JSON_NODE_OP_BOOL);
		node->u.bool = json->token.u.bool;
		forward(json);
	}
	else if (OP_NUMBER == json->token.op) {
		MKN(json, node, RA_JSON_NODE_OP_NUMBER);
		node->u.number = json->token.u.number;
		forward(json);
	}
	else if (OP_STRING == json->token.op) {
		MKN(json, node, RA_JSON_NODE_OP_STRING);
		node->u.string = json->token.u.string;
		forward(json);
	}
	else if (OP_OPEN_BRACKET == json->token.op) {
		if (!(node = jarray(json))) {
			TRACE(json, "erroneous array");
			return NULL;
		}
	}
	else if (OP_OPEN_BRACE == json->token.op) {
		if (!(node = jobject(json))) {
			TRACE(json, "erroneous object");
			return NULL;
		}
	}
	return node;
}

static struct ra_json_node *
jarray(struct ra_json *json)
{
	struct ra_json_node *node, *head, *tail;
	int mark;

	mark = 0;
	head = tail = node = NULL;
	if (OP_OPEN_BRACKET == json->token.op) {
		forward(json);
		MKN(json, node, RA_JSON_NODE_OP_ARRAY);
		head = tail = node;
		if (OP_CLOSE_BRACKET != json->token.op) {
			for (;;) {
				mark = 0;
				if (!(node->u.array.node = jvalue(json))) {
					TRACE(json, "erroneous value");
					return NULL;
				}
				if (OP_COMMA != json->token.op) {
					break;
				}
				forward(json);
				MKN(json, node, RA_JSON_NODE_OP_ARRAY);
				tail->u.array.link = node;
				tail = node;
				mark = 1;
			}
			if (mark) {
				TRACE(json, "dangling ','");
				return NULL;
			}
		}
		if (OP_CLOSE_BRACKET != json->token.op) {
			TRACE(json, "missing ']'");
			return NULL;
		}
		forward(json);
	}
	return head;
}

static struct ra_json_node *
jobject(struct ra_json *json)
{
	struct ra_json_node *node, *head, *tail;
	int mark;

	mark = 0;
	head = tail = node = NULL;
	if (OP_OPEN_BRACE == json->token.op) {
		forward(json);
		MKN(json, node, RA_JSON_NODE_OP_OBJECT);
		head = tail = node;
		for (;;) {
			if (OP_STRING != json->token.op) {
				break;
			}
			mark = 0;
			node->u.object.key = json->token.u.string;
			forward(json);
			if (OP_COLON != json->token.op) {
				TRACE(json, "missing ':'");
				return NULL;
			}
			forward(json);
			if (!(node->u.object.node = jvalue(json))) {
				TRACE(json, "erroneous value");
				return NULL;
			}
			if (OP_COMMA != json->token.op) {
				break;
			}
			forward(json);
			MKN(json, node, RA_JSON_NODE_OP_OBJECT);
			tail->u.object.link = node;
			tail = node;
			mark = 1;
		}
		if (mark) {
			TRACE(json, "dangling ','");
			return NULL;
		}
		if (OP_CLOSE_BRACE != json->token.op) {
			TRACE(json, "missing '}'");
			return NULL;
		}
		forward(json);
	}
	return head;
}

static struct ra_json_node *
jtop(struct ra_json *json)
{
	struct ra_json_node *node;

	node = NULL;
	if (OP_OPEN_BRACKET == json->token.op) {
		if (!(node = jarray(json))) {
			TRACE(json, "erroneous object");
			return NULL;
		}
	}
	else if (OP_OPEN_BRACE == json->token.op) {
		if (!(node = jobject(json))) {
			TRACE(json, "erroneous object");
			return NULL;
		}
	}
	return node;
}

ra_json_t
ra_json_open(const char *s)
{
	struct ra_json *json;

	assert( s );

	// initialize

	if (!(json = malloc(sizeof (struct ra_json)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(json, 0, sizeof (struct ra_json));
	if (!(json->content = ra_strdup(s))) {
		ra_json_close(json);
		RA_TRACE(NULL);
		return NULL;
	}

	// initialize

	json->curr = json->content;
	json->lineno = 1;
	forward(json);

	// parse

	if (!(json->root = jtop(json))) {
		TRACE(json, "empty input");
		ra_json_close(json);
		return NULL;
	}
	if (OP_END != json->token.op) {
		TRACE(json, "invalid trailing content");
		ra_json_close(json);
		return NULL;
	}
	return json;
}

void
ra_json_close(ra_json_t json)
{
	void *chunk;

	if (json) {
		while ((chunk = json->chunk)) {
			json->chunk = (*((void **)chunk));
			free(chunk);
		}
		free(json->content);
		memset(json, 0, sizeof (struct ra_json));
	}
	free(json);
}

const struct ra_json_node *
ra_json_root(ra_json_t json)
{
	assert( json );

	return json->root;
}
