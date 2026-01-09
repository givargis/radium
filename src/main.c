/* Copyright (c) Tony Givargis, 2024-2026 */

#include "lang/ra_parser.h"

static void
print(const struct ra_parser_node *node)
{
	char buf[3][32];
	const char *s;

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

		s = "";
		if (RA_PARSER_EXPR_IDENTIFIER == node->op) {
			s = node->token->u.s;
		}

		printf("%2d %2s %2s %2s %s %s\n",
		       node->id,
		       node->left ? buf[0] : "-",
		       node->right ? buf[1] : "-",
		       node->cond ? buf[2] : "-",
		       RA_PARSER_STR[node->op],
		       s);
	}
}

int
main(int argc, char *argv[])
{
	ra_parser_t parser;

	ra_trace_enabled = 1;
	ra_core_init();
	if (2 != argc) {
		fprintf(stderr, "usage: radium pathname\n");
		return -1;
	}
	if (!(parser = ra_parser_open(argv[1]))) {
		RA_TRACE("^");
		return -1;
	}
	print(ra_parser_root(parser));
	ra_parser_close(parser);
	return 0;
}
