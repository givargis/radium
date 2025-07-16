/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_json.h
 */

#ifndef _RA_JSON_H_
#define _RA_JSON_H_

#include "../kernel/ra_kernel.h"

struct ra__json_node {
	enum ra__json_node_op {
		RA__JSON_NODE_OP_,
		RA__JSON_NODE_OP_NULL,
		RA__JSON_NODE_OP_BOOL,
		RA__JSON_NODE_OP_ARRAY,
		RA__JSON_NODE_OP_OBJECT,
		RA__JSON_NODE_OP_NUMBER,
		RA__JSON_NODE_OP_STRING
	} op;
	union {
		int bool;
		ra__real_t number;
		const char *string;
		struct ra__json_node_array {
			struct ra__json_node *node;
			struct ra__json_node *link;
		} array;
		struct ra__json_node_object {
			const char *key;
			struct ra__json_node *node;
			struct ra__json_node *link;
		} object;
	} u;
};

typedef struct ra__json *ra__json_t;

ra__json_t ra__json_open(const char *s);

void ra__json_close(ra__json_t json);

const struct ra__json_node *ra__json_root(ra__json_t json);

int ra__json_bist(void);

#endif // _RA_JSON_H_
