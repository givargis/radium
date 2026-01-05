/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_json.h"

#define ERROR(j, m)					\
	do {						\
		(j)->curr = "";				\
		(j)->token.op = OP_EOF;			\
		if (!(j)->stop) {			\
			ra_printf(RA_COLOR_RED_BOLD,	\
				  "error: ");		\
			ra_printf(RA_COLOR_BLACK,	\
				  "json: "		\
				  "%u:%u: %s\n",	\
				  (j)->lineno,		\
				  (j)->column,		\
				  (m));			\
			RA_TRACE("syntax error");       \
		}					\
		(j)->stop = 1;				\
	}						\
	while (0)

#define MKN(j, n, op_)				\
	do {					\
		if (!((n) = allocate((j)))) {   \
			ERROR((j), "^");	\
			return NULL;		\
		}				\
		(n)->op = (op_);		\
	}					\
	while (0)

struct ra_json {
	int stop;
	char *curr;
	char *content;
	unsigned lineno;
	unsigned column;
	struct ra_json_node *root;
	struct {
		enum {
			OP_EOF,
			/*-*/
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
			int bool_; /* OP_BOOL */
			double number; /* OP_NUMBER */
			const char *string; /* OP_STRING */
		} u;
	} token;
	/*-*/
	struct chunk {
		struct chunk *link;
		struct ra_json_node nodes[256];
	} *chunk;
	int size;
};

static struct ra_json_node *
allocate(struct ra_json *json)
{
	struct ra_json_node *node;
	struct chunk *chunk;
	int size;

	size = (int)RA_ARRAY_SIZE(chunk->nodes);
	if (!json->chunk || (size <= json->size)) {
		if (!(chunk = malloc(sizeof (struct chunk)))) {
			RA_TRACE("out of memory");
			return NULL;
		}
		chunk->link = json->chunk;
		json->chunk = chunk;
		json->size = 0;
	}
	node = &json->chunk->nodes[++json->size];
	memset(node, 0, sizeof (struct ra_json_node));
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
	int i;

	while (*++s) {
		if ('\"' == (*s)) {
			return s;
		}
		else if (0x20 > (*s)) {
			return NULL;
		}
		else if ('\\' == (*s)) {
			++s;
			if (('"' == (*s)) ||
			    ('/' == (*s)) ||
			    ('b' == (*s)) ||
			    ('f' == (*s)) ||
			    ('n' == (*s)) ||
			    ('r' == (*s)) ||
			    ('t' == (*s)) ||
			    ('\\' == (*s))) {
				continue;
			}
			else if ('u' == (*s)) {
				for (i=0; i<4; ++i) {
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
	}
	return NULL;
}

static void
forward(struct ra_json *json)
{
	char *s, *e;
	double r;

	s = json->curr;
	while (*s) {
		if (isspace((unsigned char)(*s))) {
			if ('\n' == (*s)) {
				json->column  = 0;
				json->lineno += 1;
			}
			if ('\t' == (*s)) {
				json->column += 7;
			}
			++json->column;
			++s;
			continue;
		}
		if ('{' == (*s)) {
			json->curr = s + 1;
			json->column += 1;
			json->token.op = OP_OPEN_BRACE;
			json->token.u.string = NULL;
			return;
		}
		if ('}' == (*s)) {
			json->curr = s + 1;
			json->column += 1;
			json->token.op = OP_CLOSE_BRACE;
			json->token.u.string = NULL;
			return;
		}
		if ('[' == (*s)) {
			json->curr = s + 1;
			json->column += 1;
			json->token.op = OP_OPEN_BRACKET;
			json->token.u.string = NULL;
			return;
		}
		if (']' == (*s)) {
			json->curr = s + 1;
			json->column += 1;
			json->token.op = OP_CLOSE_BRACKET;
			json->token.u.string = NULL;
			return;
		}
		if (':' == (*s)) {
			json->curr = s + 1;
			json->column += 1;
			json->token.op = OP_COLON;
			json->token.u.string = NULL;
			return;
		}
		if (',' == (*s)) {
			json->curr = s + 1;
			json->column += 1;
			json->token.op = OP_COMMA;
			json->token.u.string = NULL;
			return;
		}
		if (!strncmp("null", s, 4)) {
			json->curr = s + 4;
			json->column += 4;
			json->token.op = OP_NULL;
			json->token.u.string = NULL;
			return;
		}
		if (!strncmp("true", s, 4)) {
			json->curr = s + 4;
			json->column += 4;
			json->token.op = OP_BOOL;
			json->token.u.bool_ = 1;
			return;
		}
		if (!strncmp("false", s, 5)) {
			json->curr = s + 5;
			json->column += 5;
			json->token.op = OP_BOOL;
			json->token.u.bool_ = 0;
			return;
		}
		if ('\"' == (*s)) {
			if (!(e = eat_string(s))) {
				ERROR(json, "erroneous string");
				return;
			}
			(*e) = '\0';
			++e;
			json->curr = e;
			json->column += (e - s);
			json->token.op = OP_STRING;
			json->token.u.string = s + 1;
			return;
		}
		if (('-' == (*s)) ||
		    ('.' == (*s)) ||
		    isdigit((unsigned char)(*s))) {
			errno = 0;
			r = strtod(s, &e);
			if ((EINVAL == errno) || (ERANGE == errno)) {
				ERROR(json, "erroneous number");
				return;
			}
			json->curr = e;
			json->column += (e - s);
			json->token.op = OP_NUMBER;
			json->token.u.number = r;
			return;
		}
		ERROR(json, "erroneous character");
		return;
	}
	json->token.op = OP_EOF;
	json->token.u.string = NULL;
}

static int
match(const struct ra_json *json, int op)
{
	if (op == (int)json->token.op) {
		return 1;
	}
	return 0;
}

static struct ra_json_node *jarray(struct ra_json *json);
static struct ra_json_node *jobject(struct ra_json *json);

static struct ra_json_node *
jvalue(struct ra_json *json)
{
	struct ra_json_node *node;

	node = NULL;
	if (match(json, OP_NULL)) {
		MKN(json, node, RA_JSON_NODE_OP_NULL);
		forward(json);
	}
	else if (match(json, OP_BOOL)) {
		MKN(json, node, RA_JSON_NODE_OP_BOOL);
		node->u.bool_ = json->token.u.bool_;
		forward(json);
	}
	else if (match(json, OP_NUMBER)) {
		MKN(json, node, RA_JSON_NODE_OP_NUMBER);
		node->u.number = json->token.u.number;
		forward(json);
	}
	else if (match(json, OP_STRING)) {
		MKN(json, node, RA_JSON_NODE_OP_STRING);
		node->u.string = json->token.u.string;
		forward(json);
	}
	else if (match(json, OP_OPEN_BRACKET)) {
		if (!(node = jarray(json))) {
			ERROR(json, "erroneous array");
			return NULL;
		}
	}
	else if (match(json, OP_OPEN_BRACE)) {
		if (!(node = jobject(json))) {
			ERROR(json, "erroneous object");
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
	if (match(json, OP_OPEN_BRACKET)) {
		forward(json);
		MKN(json, node, RA_JSON_NODE_OP_ARRAY);
		head = tail = node;
		if (!match(json, OP_CLOSE_BRACKET)) {
			for (;;) {
				mark = 0;
				if (!(node->u.array.node = jvalue(json))) {
					ERROR(json, "erroneous value");
					return NULL;
				}
				if (!match(json, OP_COMMA)) {
					break;
				}
				forward(json);
				MKN(json, node, RA_JSON_NODE_OP_ARRAY);
				tail->u.array.link = node;
				tail = node;
				mark = 1;
			}
			if (mark) {
				ERROR(json, "dangling ','");
				return NULL;
			}
		}
		if (!match(json, OP_CLOSE_BRACKET)) {
			ERROR(json, "missing ']'");
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
	if (match(json, OP_OPEN_BRACE)) {
		forward(json);
		MKN(json, node, RA_JSON_NODE_OP_OBJECT);
		head = tail = node;
		for (;;) {
			if (!match(json, OP_STRING)) {
				break;
			}
			mark = 0;
			node->u.object.key = json->token.u.string;
			forward(json);
			if (!match(json, OP_COLON)) {
				ERROR(json, "missing ':'");
				return NULL;
			}
			forward(json);
			if (!(node->u.object.node = jvalue(json))) {
				ERROR(json, "erroneous value");
				return NULL;
			}
			if (!match(json, OP_COMMA)) {
				break;
			}
			forward(json);
			MKN(json, node, RA_JSON_NODE_OP_OBJECT);
			tail->u.object.link = node;
			tail = node;
			mark = 1;
		}
		if (mark) {
			ERROR(json, "dangling ','");
			return NULL;
		}
		if (!match(json, OP_CLOSE_BRACE)) {
			ERROR(json, "missing '}'");
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
	if (match(json, OP_OPEN_BRACKET)) {
		if (!(node = jarray(json))) {
			ERROR(json, "erroneous array");
			return NULL;
		}
	}
	else if (match(json, OP_OPEN_BRACE)) {
		if (!(node = jobject(json))) {
			ERROR(json, "erroneous object");
			return NULL;
		}
	}
	return node;
}

ra_json_t
ra_json_open(const char *s)
{
	struct ra_json *json;

	/* initialize */

	if (!(json = malloc(sizeof (struct ra_json)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(json, 0, sizeof (struct ra_json));
	if (!(json->content = strdup(s))) {
		ra_json_close(json);
		RA_TRACE("out of memory");
		return NULL;
	}

	/* initialize */

	json->curr = json->content;
	json->lineno = 1;
	json->column = 1;
	forward(json);

	/* parse */

	if (!(json->root = jtop(json))) {
		ERROR(json, "empty translation unit");
		ra_json_close(json);
		return NULL;
	}
	if (!match(json, OP_EOF)) {
		ERROR(json, "superfluous content");
		ra_json_close(json);
		return NULL;
	}
	return json;
}

void
ra_json_close(ra_json_t json)
{
	struct chunk *chunk;

	if (json) {
		while ((chunk = json->chunk)) {
			json->chunk = chunk->link;
			RA_FREE(chunk);
		}
		RA_FREE(json->content);
		memset(json, 0, sizeof (struct ra_json));
		RA_FREE(json);
	}
}

const struct ra_json_node *
ra_json_root(ra_json_t json)
{
	assert( json );

	return json->root;
}

int
ra_json_array_size(const struct ra_json_node *node)
{
	uint64_t size;

	assert( node && (RA_JSON_NODE_OP_ARRAY == node->op) );

	size = 0;
	while (node) {
		size += node->u.array.node ? 1 : 0;
		node = node->u.array.link;
	}
	return size;
}

int
ra_json_test(void)
{
	const struct ra_json_node *node;
	ra_json_t json;

	/* empty array */

	if (!(json = ra_json_open("[]"))) {
		RA_TRACE("integrity failure detected");
		return -1;
	}
	if (!(node = ra_json_root(json)) ||
	    (RA_JSON_NODE_OP_ARRAY != node->op) ||
	    node->u.array.node ||
	    (node = node->u.array.link)) {
		ra_json_close(json);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	ra_json_close(json);

	/* empty object */

	if (!(json = ra_json_open("{}"))) {
		RA_TRACE("integrity failure detected");
		return -1;
	}
	if (!(node = ra_json_root(json)) ||
	    (RA_JSON_NODE_OP_OBJECT != node->op) ||
	    node->u.object.key ||
	    node->u.object.node ||
	    (node = node->u.object.link)) {
		ra_json_close(json);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	ra_json_close(json);

	/* single-element array */

	if (!(json = ra_json_open("[true]"))) {
		RA_TRACE("integrity failure detected");
		return -1;
	}
	if (!(node = ra_json_root(json)) ||
	    (RA_JSON_NODE_OP_ARRAY != node->op) ||
	    !node->u.array.node ||
	    (RA_JSON_NODE_OP_BOOL != node->u.array.node->op) ||
	    (1 != node->u.array.node->u.bool_) ||
	    (node = node->u.array.link)) {
		ra_json_close(json);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	ra_json_close(json);

	/* single-element object */

	if (!(json = ra_json_open("{\"key\":false}"))) {
		RA_TRACE("integrity failure detected");
		return -1;
	}
	if (!(node = ra_json_root(json)) ||
	    (RA_JSON_NODE_OP_OBJECT != node->op) ||
	    !node->u.object.key ||
	    strcmp("key", node->u.object.key) ||
	    !node->u.object.node ||
	    (RA_JSON_NODE_OP_BOOL != node->u.object.node->op) ||
	    (0 != node->u.object.node->u.bool_) ||
	    (node = node->u.object.link)) {
		ra_json_close(json);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	ra_json_close(json);

	/* two-element array */

	if (!(json = ra_json_open("[\"hello\",\"world\"]"))) {
		RA_TRACE("integrity failure detected");
		return -1;
	}
	if (!(node = ra_json_root(json)) ||
	    (RA_JSON_NODE_OP_ARRAY != node->op) ||
	    !node->u.array.node ||
	    (RA_JSON_NODE_OP_STRING != node->u.array.node->op) ||
	    !node->u.array.node->u.string ||
	    strcmp("hello", node->u.array.node->u.string) ||
	    !(node = node->u.array.link) ||
	    (RA_JSON_NODE_OP_ARRAY != node->op) ||
	    !node->u.array.node ||
	    (RA_JSON_NODE_OP_STRING != node->u.array.node->op) ||
	    !node->u.array.node->u.string ||
	    strcmp("world", node->u.array.node->u.string) ||
	    (node = node->u.array.link)) {
		ra_json_close(json);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	ra_json_close(json);

	/* two-element object */

	if (!(json = ra_json_open("{\"key1\":0.5,\"key2\":-.75}"))) {
		RA_TRACE("integrity failure detected");
		return -1;
	}
	if (!(node = ra_json_root(json)) ||
	    (RA_JSON_NODE_OP_OBJECT != node->op) ||
	    !node->u.object.node ||
	    !node->u.object.key ||
	    strcmp("key1", node->u.object.key) ||
	    (RA_JSON_NODE_OP_NUMBER != node->u.object.node->op) ||
	    (0.5 != node->u.object.node->u.number) ||
	    !(node = node->u.object.link) ||
	    (RA_JSON_NODE_OP_OBJECT != node->op) ||
	    !node->u.object.node ||
	    !node->u.object.key ||
	    strcmp("key2", node->u.object.key) ||
	    (RA_JSON_NODE_OP_NUMBER != node->u.object.node->op) ||
	    (-0.75 != node->u.object.node->u.number) ||
	    (node = node->u.object.link)) {
		ra_json_close(json);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	ra_json_close(json);

	/* nested */

	if (!(json = ra_json_open("[[true,false],{\"key\":\"val\"}]"))) {
		RA_TRACE("integrity failure detected");
		return -1;
	}
	if (!(node = ra_json_root(json)) ||
	    (RA_JSON_NODE_OP_ARRAY != node->op) ||
	    !(node = node->u.array.node) ||
	    (RA_JSON_NODE_OP_ARRAY != node->op) ||
	    (RA_JSON_NODE_OP_BOOL != node->u.array.node->op) ||
	    (1 != node->u.array.node->u.bool_) ||
	    !(node = node->u.array.link) ||
	    (RA_JSON_NODE_OP_BOOL != node->u.array.node->op) ||
	    (0 != node->u.array.node->u.bool_)) {
		ra_json_close(json);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	if (!(node = ra_json_root(json)) ||
	    (RA_JSON_NODE_OP_ARRAY != node->op) ||
	    !(node = node->u.array.link) ||
	    (RA_JSON_NODE_OP_ARRAY != node->op) ||
	    !(node = node->u.array.node) ||
	    (RA_JSON_NODE_OP_OBJECT != node->op) ||
	    !node->u.object.key ||
	    strcmp("key", node->u.object.key) ||
	    !(node = node->u.object.node) ||
	    (RA_JSON_NODE_OP_STRING != node->op) ||
	    strcmp("val", node->u.string)) {
		ra_json_close(json);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	ra_json_close(json);
	return 0;
}
