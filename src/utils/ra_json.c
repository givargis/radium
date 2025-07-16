/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_json.c
 */

#include "ra_json.h"

#define TRACE(j,m)					\
	do {						\
		(j)->curr = "";				\
		(j)->token.op = OP_END;			\
		ra__log("info: json:%u:%s",		\
			(j)->lineno,			\
			(m));				\
		RA__ERROR_TRACE(RA__ERROR_SYNTAX);	\
	}						\
	while (0)

#define MKN(j,n,t)				\
	do {					\
		if (!((n) = allocate((j)))) {	\
			RA__ERROR_TRACE(0);	\
			return NULL;		\
		}				\
		(n)->op = (t);			\
	}					\
	while (0)

struct ra__json {
	char *curr;
	char *content;
	unsigned lineno;
	struct ra__json_node *root;
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
			ra__real_t number;
			const char *string;
		} u;
	} token;
	void *chunk;
	uint64_t size;
};

static struct ra__json_node *
allocate(struct ra__json *json)
{
	const uint64_t CHUNK_SIZE = 4096;
	struct ra__json_node *node;
	void *chunk;
	uint64_t size;

	size = sizeof (struct ra__json_node);
	if (!json->chunk || (CHUNK_SIZE < (json->size + size))) {
		if (!(chunk = ra__malloc(CHUNK_SIZE))) {
			RA__ERROR_TRACE(0);
			return NULL;
		}
		(*((void **)chunk)) = json->chunk;
		json->size = sizeof (struct ra__json_node *);
		json->chunk = chunk;
	}
	node = (struct ra__json_node *)((char *)json->chunk + json->size);
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
forward(struct ra__json *json)
{
	ra__real_t number;
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
			number = (ra__real_t)strtold(s, &e);
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

static struct ra__json_node *jarray(struct ra__json *json);
static struct ra__json_node *jobject(struct ra__json *json);

static struct ra__json_node *
jvalue(struct ra__json *json)
{
	struct ra__json_node *node;

	node = NULL;
	if (OP_NULL == json->token.op) {
		MKN(json, node, RA__JSON_NODE_OP_NULL);
		forward(json);
	}
	else if (OP_BOOL == json->token.op) {
		MKN(json, node, RA__JSON_NODE_OP_BOOL);
		node->u.bool = json->token.u.bool;
		forward(json);
	}
	else if (OP_NUMBER == json->token.op) {
		MKN(json, node, RA__JSON_NODE_OP_NUMBER);
		node->u.number = json->token.u.number;
		forward(json);
	}
	else if (OP_STRING == json->token.op) {
		MKN(json, node, RA__JSON_NODE_OP_STRING);
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

static struct ra__json_node *
jarray(struct ra__json *json)
{
	struct ra__json_node *node, *head, *tail;
	int mark;

	mark = 0;
	head = tail = node = NULL;
	if (OP_OPEN_BRACKET == json->token.op) {
		forward(json);
		MKN(json, node, RA__JSON_NODE_OP_ARRAY);
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
				MKN(json, node, RA__JSON_NODE_OP_ARRAY);
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

static struct ra__json_node *
jobject(struct ra__json *json)
{
	struct ra__json_node *node, *head, *tail;
	int mark;

	mark = 0;
	head = tail = node = NULL;
	if (OP_OPEN_BRACE == json->token.op) {
		forward(json);
		MKN(json, node, RA__JSON_NODE_OP_OBJECT);
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
			MKN(json, node, RA__JSON_NODE_OP_OBJECT);
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

static struct ra__json_node *
jtop(struct ra__json *json)
{
	struct ra__json_node *node;

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

ra__json_t
ra__json_open(const char *s)
{
	struct ra__json *json;

	assert( s );

	// initialize

	if (!(json = ra__malloc(sizeof (struct ra__json)))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(json, 0, sizeof (struct ra__json));
	if (!(json->content = ra__strdup(s))) {
		ra__json_close(json);
		RA__ERROR_TRACE(0);
		return NULL;
	}

	// initialize

	json->curr = json->content;
	json->lineno = 1;
	forward(json);

	// parse

	if (!(json->root = jtop(json))) {
		TRACE(json, "empty input");
		ra__json_close(json);
		return NULL;
	}
	if (OP_END != json->token.op) {
		TRACE(json, "invalid trailing content");
		ra__json_close(json);
		return NULL;
	}
	return json;
}

void
ra__json_close(ra__json_t json)
{
	void *chunk;

	if (json) {
		while ((chunk = json->chunk)) {
			json->chunk = (*((void **)chunk));
			RA__FREE(chunk);
		}
		RA__FREE(json->content);
		memset(json, 0, sizeof (struct ra__json));
	}
	RA__FREE(json);
}

const struct ra__json_node *
ra__json_root(ra__json_t json)
{
	assert( json );

	return json->root;
}

int
ra__json_bist(void)
{
	const struct ra__json_node *node;
	ra__json_t json;

	// empty array

	if (!(json = ra__json_open("[]"))) {
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	if (!(node = ra__json_root(json)) ||
	    (RA__JSON_NODE_OP_ARRAY != node->op) ||
	    node->u.array.node ||
	    (node = node->u.array.link)) {
		ra__json_close(json);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	ra__json_close(json);

	// empty object

	if (!(json = ra__json_open("{}"))) {
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	if (!(node = ra__json_root(json)) ||
	    (RA__JSON_NODE_OP_OBJECT != node->op) ||
	    node->u.object.key ||
	    node->u.object.node ||
	    (node = node->u.object.link)) {
		ra__json_close(json);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	ra__json_close(json);

	// single-element array

	if (!(json = ra__json_open("[true]"))) {
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	if (!(node = ra__json_root(json)) ||
	    (RA__JSON_NODE_OP_ARRAY != node->op) ||
	    !node->u.array.node ||
	    (RA__JSON_NODE_OP_BOOL != node->u.array.node->op) ||
	    (1 != node->u.array.node->u.bool) ||
	    (node = node->u.array.link)) {
		ra__json_close(json);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	ra__json_close(json);

	// single-element object

	if (!(json = ra__json_open("{\"key\":false}"))) {
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	if (!(node = ra__json_root(json)) ||
	    (RA__JSON_NODE_OP_OBJECT != node->op) ||
	    !node->u.object.key ||
	    strcmp("key", node->u.object.key) ||
	    !node->u.object.node ||
	    (RA__JSON_NODE_OP_BOOL != node->u.object.node->op) ||
	    (0 != node->u.object.node->u.bool) ||
	    (node = node->u.object.link)) {
		ra__json_close(json);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	ra__json_close(json);

	// two-element array

	if (!(json = ra__json_open("[\"hello\",\"world\"]"))) {
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	if (!(node = ra__json_root(json)) ||
	    (RA__JSON_NODE_OP_ARRAY != node->op) ||
	    !node->u.array.node ||
	    (RA__JSON_NODE_OP_STRING != node->u.array.node->op) ||
	    !node->u.array.node->u.string ||
	    strcmp("hello", node->u.array.node->u.string) ||
	    !(node = node->u.array.link) ||
	    (RA__JSON_NODE_OP_ARRAY != node->op) ||
	    !node->u.array.node ||
	    (RA__JSON_NODE_OP_STRING != node->u.array.node->op) ||
	    !node->u.array.node->u.string ||
	    strcmp("world", node->u.array.node->u.string) ||
	    (node = node->u.array.link)) {
		ra__json_close(json);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	ra__json_close(json);

	// two-element object

	if (!(json = ra__json_open("{\"key1\":0.5,\"key2\":-.75}"))) {
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	if (!(node = ra__json_root(json)) ||
	    (RA__JSON_NODE_OP_OBJECT != node->op) ||
	    !node->u.object.node ||
	    !node->u.object.key ||
	    strcmp("key1", node->u.object.key) ||
	    (RA__JSON_NODE_OP_NUMBER != node->u.object.node->op) ||
	    (0.5 != node->u.object.node->u.number) ||
	    !(node = node->u.object.link) ||
	    (RA__JSON_NODE_OP_OBJECT != node->op) ||
	    !node->u.object.node ||
	    !node->u.object.key ||
	    strcmp("key2", node->u.object.key) ||
	    (RA__JSON_NODE_OP_NUMBER != node->u.object.node->op) ||
	    (-0.75 != node->u.object.node->u.number) ||
	    (node = node->u.object.link)) {
		ra__json_close(json);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	ra__json_close(json);

	// nested

	if (!(json = ra__json_open("[[true,false],{\"key\":\"val\"}]"))) {
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	if (!(node = ra__json_root(json)) ||
	    (RA__JSON_NODE_OP_ARRAY != node->op) ||
	    !(node = node->u.array.node) ||
	    (RA__JSON_NODE_OP_ARRAY != node->op) ||
	    (RA__JSON_NODE_OP_BOOL != node->u.array.node->op) ||
	    (1 != node->u.array.node->u.bool) ||
	    !(node = node->u.array.link) ||
	    (RA__JSON_NODE_OP_BOOL != node->u.array.node->op) ||
	    (0 != node->u.array.node->u.bool)) {
		ra__json_close(json);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	if (!(node = ra__json_root(json)) ||
	    (RA__JSON_NODE_OP_ARRAY != node->op) ||
	    !(node = node->u.array.link) ||
	    (RA__JSON_NODE_OP_ARRAY != node->op) ||
	    !(node = node->u.array.node) ||
	    (RA__JSON_NODE_OP_OBJECT != node->op) ||
	    !node->u.object.key ||
	    strcmp("key", node->u.object.key) ||
	    !(node = node->u.object.node) ||
	    (RA__JSON_NODE_OP_STRING != node->op) ||
	    strcmp("val", node->u.string)) {
		ra__json_close(json);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	ra__json_close(json);
	return 0;
}
