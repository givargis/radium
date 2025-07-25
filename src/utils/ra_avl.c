/**
 * Copyright (c) Tony Givargis, 2024-2025
 *
 * ra_avl.c
 */

#include "ra_avl.h"

struct ra_avl {
	uint64_t items;
	struct node {
		int depth;
		const char *key;
		const void *val;
		struct node *left;
		struct node *right;
	} *root;
};

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
min(struct node *node)
{
	struct node *node_;

	node_ = node;
	while (node) {
		node_ = node;
		node = node->left;
	}
	return node_;
}

static void
destroy(struct node *root)
{
	if (root) {
		destroy(root->left);
		destroy(root->right);
		free((void *)root->key);
		memset(root, 0, sizeof (struct node));
	}
	free(root);
}

static struct node *
delete(struct node *root, const char *key, int *found)
{
	struct node *node, temp;
	int d;

	if (root) {
		if (!(d = strcmp(key, root->key))) {
			if (!root->left || !root->right) {
				node = root->left ? root->left : root->right;
				if (!node) {
					destroy(root);
					root = NULL;
				}
				else {
					temp = *root;
					(*root) = (*node);
					(*node) = temp;
					node->left = NULL;
					node->right = NULL;
					destroy(node);
				}
			}
			else {
				node = min(root->right);
				free((void *)root->key);
				if (!(root->key = ra_strdup(node->key))) {
					RA_TRACE(NULL);
					exit(-1);
				}
				root->val = node->val;
				node->val = NULL;
				root->right = delete(root->right,
						     node->key,
						     found);
			}
			(*found) = 1;
		}
		else if (0 > d) {
			root->left = delete(root->left, key, found);
		}
		else if (0 < d) {
			root->right = delete(root->right, key, found);
		}
	}
	if (root) {
		root->depth = depth(root->left, root->right);
		if ((1 < balance(root)) && (0 <= balance(root->left))) {
			root = rotate_right(root);
		}
		else if ((1 < balance(root)) && (0 > balance(root->left))) {
			root = rotate_left_right(root);
		}
		else if ((-1 > balance(root)) && (0 >= balance(root->right))) {
			root = rotate_left(root);
		}
		else if ((-1 > balance(root)) && (0 < balance(root->right))) {
			root = rotate_right_left(root);
		}
	}
	return root;
}

static struct node *
update(struct ra_avl *avl, struct node *root, const char *key, const void *val)
{
	int d;

	if (!root) {
		if (!(root = malloc(sizeof (struct node)))) {
			RA_TRACE("out of memory");
			return NULL;
		}
		memset(root, 0, sizeof (struct node));
		if (!(root->key = ra_strdup(key))) {
			RA_TRACE(NULL);
			return NULL;
		}
		root->val = val;
		++avl->items;
		return root;
	}
	if (!(d = strcmp(key, root->key))) {
		root->val = val;
	}
	else if (0 > d) {
		root->left = update(avl, root->left, key, val);
		if (1 < abs(balance(root))) {
			if (0 > strcmp(key, root->left->key)) {
				root = rotate_right(root);
			}
			else {
				root = rotate_left_right(root);
			}
		}
	}
	else if (0 < d) {
		root->right = update(avl, root->right, key, val);
		if (1 < abs(balance(root))) {
			if (0 < strcmp(key, root->right->key)) {
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

static int
iterate(struct node *root, ra_avl_fnc_t fnc, void *ctx)
{
	int e;

	if (root) {
		if ((e = iterate(root->left, fnc, ctx)) ||
		    (e = fnc(ctx, root->key, (void *)root->val)) ||
		    (e = iterate(root->right, fnc, ctx))) {
			return e;
		}
	}
	return 0;
}

ra_avl_t
ra_avl_open(void)
{
	struct ra_avl *avl;

	if (!(avl = malloc(sizeof (struct ra_avl)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(avl, 0, sizeof (struct ra_avl));
	return avl;
}

void
ra_avl_close(ra_avl_t avl)
{
	if (avl) {
		destroy(avl->root);
		memset(avl, 0, sizeof (struct ra_avl));
	}
	free(avl);
}

void
ra_avl_empty(ra_avl_t avl)
{
	assert( avl );

	destroy(avl->root);
	memset(avl, 0, sizeof (struct ra_avl));
}

void
ra_avl_delete(ra_avl_t avl, const char *key)
{
	int found;

	assert( avl );
	assert( key && strlen(key) );

	found = 0;
	avl->root = delete(avl->root, key, &found);
	avl->items -= found ? 1 : 0;
}

int
ra_avl_update(ra_avl_t avl, const char *key, const void *val)
{
	struct node *root;

	assert( avl );
	assert( key && strlen(key) );

	if (!(root = update(avl, avl->root, key, val))) {
		RA_TRACE(NULL);
		return -1;
	}
	avl->root = root;
	return 0;
}

void *
ra_avl_lookup(ra_avl_t avl, const char *key)
{
	const struct node *node;
	int d;

	assert( avl );
	assert( key && strlen(key) );

	node = avl->root;
	while (node) {
		if (!(d = strcmp(key, node->key))) {
			return (void *)node->val;
		}
		node = (0 > d) ? node->left : node->right;
	}
	return NULL;
}

int
ra_avl_iterate(ra_avl_t avl, ra_avl_fnc_t fnc, void *ctx)
{
	int e;

	assert( avl );
	assert( fnc );

	if ((e = iterate(avl->root, fnc, ctx))) {
		return e;
	}
	return 0;
}

uint64_t
ra_avl_items(ra_avl_t avl)
{
	assert( avl );

	return avl->items;
}

int
ra_avl_bist(void)
{
	const int N = 123456;
	const void *val_;
	ra_avl_t avl;
	char key[32];
	char val[32];

	// initialize

	if (!(avl = ra_avl_open())) {
		RA_TRACE(NULL);
		return -1;
	}
	if (0 != ra_avl_items(avl)) {
		ra_avl_close(avl);
		RA_TRACE("software");
		return -1;
	}

	// single item

	if (ra_avl_update(avl, "key", "val") ||
	    (1 != ra_avl_items(avl)) ||
	    strcmp("val",
		   ra_avl_lookup(avl, "key") ?
		   ra_avl_lookup(avl, "key") : "") ||
	    ra_avl_update(avl, "key", "lav") ||
	    (1 != ra_avl_items(avl)) ||
	    strcmp("lav",
		   ra_avl_lookup(avl, "key") ?
		   ra_avl_lookup(avl, "key") : "")) {
		ra_avl_close(avl);
		RA_TRACE("software");
		return -1;
	}
	ra_avl_delete(avl, "key");
	if (0 != ra_avl_items(avl)) {
		ra_avl_close(avl);
		RA_TRACE("software");
		return -1;
	}

	// random update

	srand(10);
	for (int i=0; i<N; ++i) {
		ra_sprintf(key, sizeof (key), "%d-%d", rand(), i);
		ra_sprintf(val, sizeof (val), "%d-%d", rand(), i);
		if (ra_avl_update(avl, key, val)) {
			ra_avl_close(avl);
			RA_TRACE("software");
			return -1;
		}
		val_ = ra_avl_lookup(avl, key);
		if (!val_ || strcmp(val_, val)) {
			ra_avl_close(avl);
			RA_TRACE("software");
			return -1;
		}
	}

	// random lookup

	srand(10);
	for (int i=0; i<N; ++i) {
		ra_sprintf(key, sizeof (key), "%d-%d", rand(), i);
		ra_sprintf(val, sizeof (val), "%d-%d", rand(), i);
		val_ = ra_avl_lookup(avl, key);
		if (!val_ || strcmp(val_, val)) {
			ra_avl_close(avl);
			RA_TRACE("software");
			return -1;
		}
	}

	// random delete

	srand(10);
	for (int i=0; i<(N/2); ++i) {
		ra_sprintf(key, sizeof (key), "%d-%d", rand(), i);
		ra_sprintf(val, sizeof (val), "%d-%d", rand(), i);
		ra_avl_delete(avl, key);
		if (ra_avl_lookup(avl, key)) {
			ra_avl_close(avl);
			RA_TRACE("software");
			return -1;
		}
	}

	// random lookup

	srand(10);
	for (int i=0; i<N; ++i) {
		ra_sprintf(key, sizeof (key), "%d-%d", rand(), i);
		ra_sprintf(val, sizeof (val), "%d-%d", rand(), i);
		val_ = ra_avl_lookup(avl, key);
		if (i < (N / 2)) {
			if (val_) {
				ra_avl_close(avl);
				RA_TRACE("software");
				return -1;
			}
		}
		else {
			if (!val_ || strcmp(val_, val)) {
				ra_avl_close(avl);
				RA_TRACE("software");
				return -1;
			}
		}
	}

	// empty

	ra_avl_empty(avl);
	if (0 != ra_avl_items(avl)) {
		ra_avl_close(avl);
		RA_TRACE("software");
		return -1;
	}

	// done

	ra_avl_close(avl);
	return 0;
}
