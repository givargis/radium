/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_index_ternary.c
 */

#include "ra_index_queue.h"
#include "ra_index_ternary.h"

#define CHAR2INT(c) ( (int)((unsigned char)(c)) )

struct node {
	char key;
	int valid;
	uint64_t ref;
	struct node *left;
	struct node *right;
	struct node *center;
};

struct ra__index_ternary {
	void *chunk;
	uint64_t size;
	/*-*/
	uint64_t items;
	uint64_t nodes;
	struct node *root;
};

static int
check(struct ra__index_ternary *ternary, uint64_t n)
{
	const uint64_t CHUNK_SIZE = 1048576;
	void *chunk;

	if (!ternary->chunk || (CHUNK_SIZE < (ternary->size + n))) {
		if (!(chunk = ra__malloc(CHUNK_SIZE))) {
			RA__ERROR_TRACE(0);
			return -1;
		}
		(*((void **)chunk)) = ternary->chunk;
		ternary->size = sizeof (struct node);
		ternary->chunk = chunk;
	}
	return 0;
}

static struct node *
get_node(const struct ra__index_ternary *ternary, uint64_t i)
{
	return (struct node *)((char *)ternary->chunk + i);
}

static void
update(struct ra__index_ternary *ternary,
       struct node *root,
       const char *key,
       uint64_t ref)
{
	struct node *parent;
	int s, d;

	s = 0;
	parent = NULL;
	for (;;) {
		if (!root) {
			root = get_node(ternary, ternary->size);
			memset(root, 0, (sizeof (struct node)));
			root->key = (*key);
			ternary->size += (sizeof (struct node));
			ternary->nodes += 1;
			if (!ternary->root) {
				ternary->root = root;
			}
			switch (s) {
			case 1: parent->left = root; break;
			case 2: parent->center = root; break;
			case 3: parent->right = root; break;
			}
		}
		parent = root;
		d = CHAR2INT(*key) - CHAR2INT(root->key);
		if (!d) {
			if ('\0' == (*(++key))) {
				root->valid = 1;
				root->ref = ref;
				ternary->items += 1;
				break;
			}
			root = root->center;
			s = 2;
		}
		else if (0 > d) {
			root = root->left;
			s = 1;
		}
		else {
			root = root->right;
			s = 3;
		}
	}
}

static int
_encode_(void *ctx, const char *key, uint64_t ref)
{
	struct ra__index_ternary *ternary;

	ternary = (struct ra__index_ternary *)ctx;
	if (check(ternary, sizeof (struct node) * ra__strlen(key))) {
		RA__ERROR_TRACE(0);
		return -1;
	}
	update(ternary, ternary->root, key, ref);
	return 0;
}

ra__index_ternary_t
ra__index_ternary_open(ra__index_tree_t tree)
{
	struct ra__index_ternary *ternary;

	assert( tree );

	if (!(ternary = ra__malloc(sizeof (struct ra__index_ternary)))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(ternary, 0, sizeof (struct ra__index_ternary));
	if (ra__index_tree_iterate(tree, _encode_, ternary)) {
		ra__index_ternary_close(ternary);
		RA__ERROR_TRACE(0);
		return NULL;
	}
	return ternary;
}

void
ra__index_ternary_close(ra__index_ternary_t ternary)
{
	void *chunk;

	if (ternary) {
		while ((chunk = ternary->chunk)) {
			ternary->chunk = (*((void **)chunk));
			RA__FREE(chunk);
		}
		memset(ternary, 0, sizeof (struct ra__index_ternary));
	}
	RA__FREE(ternary);
}

int
ra__index_ternary_iterate(ra__index_ternary_t ternary,
			  ra__index_ternary_fnc_t fnc,
			  void *ctx)
{
	ra__index_queue_t queue;
	struct node *node;

	assert( ternary );
	assert( fnc );

	if (ternary->root) {
		if (!(queue = ra__index_queue_open(ternary->nodes))) {
			RA__ERROR_TRACE(0);
			return -1;
		}
		ra__index_queue_push(queue, ternary->root);
		while (!ra__index_queue_empty(queue)) {
			node = ra__index_queue_pop(queue);
			fnc(ctx,
			    node->key,
			    (node->left ? 1 : 0),
			    (node->center ? 1 : 0),
			    (node->right ? 1 : 0),
			    node->valid ? &node->ref : NULL);
			if (node->left) {
				ra__index_queue_push(queue, node->left);
			}
			if (node->center) {
				ra__index_queue_push(queue, node->center);
			}
			if (node->right) {
				ra__index_queue_push(queue, node->right);
			}
		}
		ra__index_queue_close(queue);
	}
	return 0;
}

uint64_t
ra__index_ternary_items(ra__index_ternary_t ternary)
{
	assert( ternary );

	return ternary->items;
}

uint64_t
ra__index_ternary_nodes(ra__index_ternary_t ternary)
{
	assert( ternary );

	return ternary->nodes;
}
