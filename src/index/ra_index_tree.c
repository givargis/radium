/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_index_tree.c
 */

#include "ra_index_queue.h"
#include "ra_index_tree.h"

#pragma pack(push, 1)
struct node {
	int depth;
	uint64_t ref;
	struct node *left;
	struct node *right;
};
#pragma pack(pop)

struct ra__index_tree {
	void *chunk;
	uint64_t size;
	/*-*/
	void *root;
	uint64_t items;
};

static int
check(struct ra__index_tree *tree, uint64_t n)
{
	const uint64_t CHUNK_SIZE = 1048576;
	void *chunk;

	if (!tree->chunk || (CHUNK_SIZE < (tree->size + n))) {
		if (!(chunk = ra__malloc(CHUNK_SIZE))) {
			RA__ERROR_TRACE(0);
			return -1;
		}
		(*((void **)chunk)) = tree->chunk;
		tree->size = sizeof (void *);
		tree->chunk = chunk;
	}
	return 0;
}

static struct node *
get_node(struct ra__index_tree *tree, uint64_t i)
{
	return (struct node *)((char *)tree->chunk + i);
}

static const char *
get_key(const struct node *node)
{
	return (const char *)(node + 1);
}

static int
delta(const struct node *node)
{
	return node ? node->depth : -1;
}

static int
balance(const struct node *node)
{
	return delta(node->left) - delta(node->right);
}

static int
depth(const struct node *a, const struct node *b)
{
	return (delta(a) > delta(b)) ? (delta(a) + 1) : (delta(b) + 1);
}

static struct node *
rotate_right(struct node *node)
{
	struct node *root;

	root = node->left;
	node->left = root->right;
	root->right = node;
	node->depth = depth(node->left, node->right);
	root->depth = depth(root->left, node);
	return root;
}

static struct node *
rotate_left(struct node *node)
{
	struct node *root;

	root = node->right;
	node->right = root->left;
	root->left = node;
	node->depth = depth(node->left, node->right);
	root->depth = depth(root->right, node);
	return root;
}

static struct node *
rotate_left_right(struct node *node)
{
	node->left = rotate_left(node->left);
	return rotate_right(node);
}

static struct node *
rotate_right_left(struct node *node)
{
	node->right = rotate_right(node->right);
	return rotate_left(node);
}

static struct node *
update(struct ra__index_tree *tree,
       struct node *root,
       const char *key,
       uint64_t **ref)
{
	int d;

	if (!root) {
		root = get_node(tree, tree->size);
		memset(root, 0, sizeof (struct node));
		memcpy(root + 1, key, ra__strlen(key) + 1);
		tree->size += sizeof (struct node) + ra__strlen(key) + 1;
		tree->items += 1;
		(*ref) = &root->ref;
		return root;
	}
	if (!(d = strcmp(key, get_key(root)))) {
		(*ref) = &root->ref;
	}
	else if (0 > d) {
		root->left = update(tree, root->left, key, ref);
		if (1 < abs(balance(root))) {
			if (0 > strcmp(key, get_key(root->left))) {
				root = rotate_right(root);
			}
			else {
				root = rotate_left_right(root);
			}
		}
	}
	else if (0 < d) {
		root->right = update(tree, root->right, key, ref);
		if (1 < abs(balance(root))) {
			if (0 < strcmp(key, get_key(root->right))) {
				root = rotate_left(root);
			}
			else {
				root = rotate_right_left(root);
			}
		}
	}
	root->depth = depth(root->left, root->right);
	return root;
}

static struct node *
min(struct node *root)
{
	while (root->left) {
		root = root->left;
	}
	return root;
}

static struct node *
max(struct node *root)
{
	while (root->right) {
		root = root->right;
	}
	return root;
}

static struct node *
next(struct node *root, const char *key)
{
	struct node *node;
	int d;

	node = NULL;
	while (root) {
		if (!(d = strcmp(key, get_key(root)))) {
			if (root->right) {
				return min(root->right);
			}
			break;
		}
		else if (0 > d) {
			node = root;
			root = root->left;
		}
		else {
			root = root->right;
		}
	}
	return node;
}

static struct node *
prev(struct node *root, const void *key)
{
	struct node *node;
	int d;

	node = NULL;
	while (root) {
		if (!(d = strcmp(key, get_key(root)))) {
			if (root->left) {
				return max(root->left);
			}
			break;
		}
		else if (0 > d) {
			root = root->left;
		}
		else {
			node = root;
			root = root->right;
		}
	}
	return node;
}

int
ra__index_tree_iterate(ra__index_tree_t tree, ra__index_tree_fnc_t fnc, void *ctx)
{
	struct node *node;
	ra__index_queue_t queue;

	assert( tree );
	assert( fnc );

	if (tree->root) {
		if (!(queue = ra__index_queue_open(tree->items))) {
			RA__ERROR_TRACE(0);
			return -1;
		}
		ra__index_queue_push(queue, tree->root);
		while (!ra__index_queue_empty(queue)) {
			node = ra__index_queue_pop(queue);
			if (fnc(ctx, get_key(node), node->ref)) {
				ra__index_queue_close(queue);
				RA__ERROR_TRACE(0);
				return -1;
			}
			if (node->left) {
				ra__index_queue_push(queue, node->left);
			}
			if (node->right) {
				ra__index_queue_push(queue, node->right);
			}
		}
		ra__index_queue_close(queue);
	}
	return 0;
}

ra__index_tree_t
ra__index_tree_open(void)
{
	struct ra__index_tree *tree;

	if (!(tree = ra__malloc(sizeof (struct ra__index_tree)))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(tree, 0, sizeof (struct ra__index_tree));
	return tree;
}

void
ra__index_tree_close(ra__index_tree_t tree)
{
	void *chunk;

	if (tree) {
		while ((chunk = tree->chunk)) {
			tree->chunk = (*((void **)chunk));
			RA__FREE(chunk);
		}
		memset(tree, 0, sizeof (struct ra__index_tree));
	}
	RA__FREE(tree);
}

void
ra__index_tree_truncate(ra__index_tree_t tree)
{
	void *chunk;

	if (tree) {
		while ((chunk = tree->chunk)) {
			tree->chunk = (*((void **)chunk));
			RA__FREE(chunk);
		}
		memset(tree, 0, sizeof (struct ra__index_tree));
	}
}

uint64_t *
ra__index_tree_update(ra__index_tree_t tree, const char *key)
{
	uint64_t *ref;

	assert( tree );
	assert( ra__strlen(key) );
	assert( RA__INDEX_TREE_MAX_KEY_LEN > ra__strlen(key) );

	if (check(tree, sizeof (struct node) + ra__strlen(key) + 1)) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	tree->root = update(tree, tree->root, key, &ref);
	return ref;
}

uint64_t *
ra__index_tree_find(ra__index_tree_t tree, const char *key)
{
	struct node *node;
	int d;

	assert( tree );
	assert( ra__strlen(key) );

	node = tree->root;
	while (node) {
		if (!(d = strcmp(key, get_key(node)))) {
			return &node->ref;
		}
		node = (0 > d) ? node->left : node->right;
	}
	return NULL;
}

uint64_t *
ra__index_tree_next(ra__index_tree_t tree, const char *key, char *okey)
{
	struct node *node;

	assert( tree );
	assert( okey );

	if (ra__strlen(key)) {
		if ((node = next(tree->root, key))) {
			memcpy(okey,
			       get_key(node),
			       ra__strlen(get_key(node)) + 1);
			return &node->ref;
		}
	}
	else if (tree->root) {
		if ((node = min(tree->root))) {
			memcpy(okey,
			       get_key(node),
			       ra__strlen(get_key(node)) + 1);
			return &node->ref;
		}
	}
	return NULL;
}

uint64_t *
ra__index_tree_prev(ra__index_tree_t tree, const char *key, char *okey)
{
	struct node *node;

	assert( tree );
	assert( okey );

	if (ra__strlen(key)) {
		if ((node = prev(tree->root, key))) {
			memcpy(okey,
			       get_key(node),
			       ra__strlen(get_key(node)) + 1);
			return &node->ref;
		}
	}
	else if (tree->root) {
		if ((node = max(tree->root))) {
			memcpy(okey,
			       get_key(node),
			       ra__strlen(get_key(node)) + 1);
			return &node->ref;
		}
	}
	return NULL;
}

uint64_t
ra__index_tree_items(ra__index_tree_t tree)
{
	assert( tree );

	return tree->items;
}
