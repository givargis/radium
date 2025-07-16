/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_avl.c
 */

#include "ra_avl.h"

struct ra__avl {
	uint64_t size;
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
		RA__FREE(root->key);
		memset(root, 0, sizeof (struct node));
	}
	RA__FREE(root);
}

static struct node *
remove_(struct node *root, const char *key, int *found)
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
				RA__FREE(root->key);
				if (!(root->key = ra__strdup(node->key))) {
					RA__ERROR_HALT(0);
					return NULL;
				}
				root->val = node->val;
				node->val = NULL;
				root->right = remove_(root->right,
						      node->key,
						      found);
			}
			(*found) = 1;
		}
		else if (0 > d) {
			root->left = remove_(root->left, key, found);
		}
		else if (0 < d) {
			root->right = remove_(root->right, key, found);
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
update(struct ra__avl *avl, struct node *root, const char *key, const void *val)
{
	int d;

	if (!root) {
		if (!(root = ra__malloc(sizeof (struct node)))) {
			RA__ERROR_TRACE(0);
			return NULL;
		}
		memset(root, 0, sizeof (struct node));
		if (!(root->key = ra__strdup(key))) {
			RA__ERROR_TRACE(0);
			return NULL;
		}
		root->val = val;
		++avl->size;
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
iterate_(struct node *root, ra__avl_fnc_t fnc, void *ctx)
{
	int e;

	if (root) {
		if ((e = iterate_(root->left, fnc, ctx)) ||
		    (e = fnc(ctx, root->key, (void *)root->val)) ||
		    (e = iterate_(root->right, fnc, ctx))) {
			return e;
		}
	}
	return 0;
}

ra__avl_t
ra__avl_open(void)
{
	struct ra__avl *avl;

	if (!(avl = ra__malloc(sizeof (struct ra__avl)))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(avl, 0, sizeof (struct ra__avl));
	return avl;
}

void
ra__avl_close(ra__avl_t avl)
{
	if (avl) {
		destroy(avl->root);
		memset(avl, 0, sizeof (struct ra__avl));
	}
	RA__FREE(avl);
}

void
ra__avl_empty(ra__avl_t avl)
{
	assert( avl );

	destroy(avl->root);
	memset(avl, 0, sizeof (struct ra__avl));
}

void
ra__avl_remove(ra__avl_t avl, const char *key)
{
	int found;

	assert( avl );
	assert( ra__strlen(key) );

	found = 0;
	avl->root = remove_(avl->root, key, &found);
	avl->size -= found ? 1 : 0;
}

int
ra__avl_update(ra__avl_t avl, const char *key, const void *val)
{
	struct node *root;

	assert( avl );
	assert( ra__strlen(key) );

	if (!(root = update(avl, avl->root, key, val))) {
		RA__ERROR_TRACE(0);
		return -1;
	}
	avl->root = root;
	return 0;
}

void *
ra__avl_lookup(ra__avl_t avl, const char *key)
{
	const struct node *node;
	int d;

	assert( avl );
	assert( ra__strlen(key) );

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
ra__avl_iterate(ra__avl_t avl, ra__avl_fnc_t fnc, void *ctx)
{
	int e;

	assert( avl );
	assert( fnc );

	if ((e = iterate_(avl->root, fnc, ctx))) {
		return e;
	}
	return 0;
}

uint64_t
ra__avl_size(ra__avl_t avl)
{
	assert( avl );

	return avl->size;
}

int
ra__avl_bist(void)
{
	const int N = 123456;
	const void *val_;
	ra__avl_t avl;
	char key[32];
	char val[32];
	int i;

	// initialize

	if (!(avl = ra__avl_open())) {
		RA__ERROR_TRACE(0);
		return -1;
	}
	if (0 != ra__avl_size(avl)) {
		ra__avl_close(avl);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}

	// single item

	if (ra__avl_update(avl, "key", "val") ||
	    (1 != ra__avl_size(avl)) ||
	    strcmp("val",
		   ra__avl_lookup(avl, "key") ?
		   ra__avl_lookup(avl, "key") : "") ||
	    ra__avl_update(avl, "key", "lav") ||
	    (1 != ra__avl_size(avl)) ||
	    strcmp("lav",
		   ra__avl_lookup(avl, "key") ?
		   ra__avl_lookup(avl, "key") : "")) {
		ra__avl_close(avl);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	ra__avl_remove(avl, "key");
	if (0 != ra__avl_size(avl)) {
		ra__avl_close(avl);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}

	// random update

	srand(10);
	for (i=0; i<N; ++i) {
		ra__sprintf(key, sizeof (key), "%d-%d", rand(), i);
		ra__sprintf(val, sizeof (val), "%d-%d", rand(), i);
		if (ra__avl_update(avl, key, val)) {
			ra__avl_close(avl);
			RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
			return -1;
		}
		val_ = ra__avl_lookup(avl, key);
		if (!val_ || strcmp(val_, val)) {
			ra__avl_close(avl);
			RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
			return -1;
		}
	}
	if (i != (int)ra__avl_size(avl)) {
		ra__avl_close(avl);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}

	// random lookup

	srand(10);
	for (i=0; i<N; ++i) {
		ra__sprintf(key, sizeof (key), "%d-%d", rand(), i);
		ra__sprintf(val, sizeof (val), "%d-%d", rand(), i);
		val_ = ra__avl_lookup(avl, key);
		if (!val_ || strcmp(val_, val)) {
			ra__avl_close(avl);
			RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
			return -1;
		}
	}

	// random remove

	srand(10);
	for (i=0; i<(N/2); ++i) {
		ra__sprintf(key, sizeof (key), "%d-%d", rand(), i);
		ra__sprintf(val, sizeof (val), "%d-%d", rand(), i);
		ra__avl_remove(avl, key);
		if (ra__avl_lookup(avl, key)) {
			ra__avl_close(avl);
			RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
			return -1;
		}
	}
	if (i != (int)ra__avl_size(avl)) {
		ra__avl_close(avl);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}

	// random lookup

	srand(10);
	for (i=0; i<N; ++i) {
		ra__sprintf(key, sizeof (key), "%d-%d", rand(), i);
		ra__sprintf(val, sizeof (val), "%d-%d", rand(), i);
		val_ = ra__avl_lookup(avl, key);
		if (i < (N / 2)) {
			if (val_) {
				ra__avl_close(avl);
				RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
				return -1;
			}
		}
		else {
			if (!val_ || strcmp(val_, val)) {
				ra__avl_close(avl);
				RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
				return -1;
			}
		}
	}

	// empty

	ra__avl_empty(avl);
	if (0 != ra__avl_size(avl)) {
		ra__avl_close(avl);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}

	// done

	ra__avl_close(avl);
	return 0;
}
