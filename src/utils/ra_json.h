/**
 * Copyright (c) Tony Givargis, 2024-2025
 *
 * ra_json.h
 */

#ifndef _RA_JSON_H_
#define _RA_JSON_H_

#include "../root/ra_root.h"

struct ra_json_node {
	enum ra_json_node_op {
		RA_JSON_NODE_OP_,
		RA_JSON_NODE_OP_NULL,
		RA_JSON_NODE_OP_BOOL,
		RA_JSON_NODE_OP_ARRAY,
		RA_JSON_NODE_OP_OBJECT,
		RA_JSON_NODE_OP_NUMBER,
		RA_JSON_NODE_OP_STRING
	} op;
	union {
		int bool;
		double number;
		const char *string;
		struct ra_json_node_array {
			struct ra_json_node *node;
			struct ra_json_node *link;
		} array;
		struct ra_json_node_object {
			const char *key;
			struct ra_json_node *node;
			struct ra_json_node *link;
		} object;
	} u;
};

typedef struct ra_json *ra_json_t;

ra_json_t ra_json_open(const char *s);

void ra_json_close(ra_json_t json);

const struct ra_json_node *ra_json_root(ra_json_t json);

int ra_json_bist(void);

#endif // _RA_JSON_H_
