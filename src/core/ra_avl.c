/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_avl.h"

struct ra_avl {
	uint64_t items;
	struct node {
		int depth;
		const char *key;
		const void *val; /* caller managed */
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
	root->depth = depth(root->left, node);
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

static void
destroy(struct node *root)
{
	if (root) {
		destroy(root->left);
		destroy(root->right);
		RA_FREE(root->key);
		memset(root, 0, sizeof (struct node));
		RA_FREE(root);
	}
}

static struct node *
update(struct ra_avl *avl, struct node *root, const char *key, const void *val)
{
	struct node *node;
	int d;

	if (!root) {
		if (!(root = malloc(sizeof (struct node)))) {
			RA_TRACE("out of memory");
			return NULL;
		}
		memset(root, 0, sizeof (struct node));
		if (!(root->key = strdup(key))) {
			RA_FREE(root);
			RA_TRACE("out of memory");
			return NULL;
		}
		root->val = val;
		++avl->items;
		return root;
	}
	if (!(d = strcmp(key, root->key))) {
		root->val = val;
		return root;
	}
	if (0 > d) {
		if (!(node = update(avl, root->left, key, val))) {
			RA_TRACE("^");
			return NULL;
		}
		root->left = node;
		if (1 < balance(root)) {
			if (0 <= balance(root->left)) {
				root = rotate_right(root);
			} else {
				root = rotate_left_right(root);
			}
		}
	} else {
		if (!(node = update(avl, root->right, key, val))) {
			RA_TRACE("^");
			return NULL;
		}
		root->right = node;
		if (-1 > balance(root)) {
			if (0 >= balance(root->right)) {
				root = rotate_left(root);
			} else {
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
		RA_FREE(avl);
	}
}

void
ra_avl_empty(ra_avl_t avl)
{
	if (avl) {
		destroy(avl->root);
		memset(avl, 0, sizeof (struct ra_avl));
	}
}

int
ra_avl_update(ra_avl_t avl, const char *key, const void *val)
{
	struct node *root;

	assert( avl );
	assert( key && strlen(key) );

	if (!(root = update(avl, avl->root, key, val))) {
		RA_TRACE("^");
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
