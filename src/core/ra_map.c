/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_map.h"

struct ra_map {
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
update(struct ra_map *map, struct node *root, const char *key, const void *val)
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
		++map->items;
		return root;
	}
	if (!(d = strcmp(key, root->key))) {
		root->val = val;
		return root;
	}
	if (0 > d) {
		if (!(node = update(map, root->left, key, val))) {
			RA_TRACE("^");
			return NULL;
		}
		root->left = node;
		if (1 < balance(root)) {
			if (0 <= balance(root->left)) {
				root = rotate_right(root);
			}
			else {
				root = rotate_left_right(root);
			}
		}
	}
	else {
		if (!(node = update(map, root->right, key, val))) {
			RA_TRACE("^");
			return NULL;
		}
		root->right = node;
		if (-1 > balance(root)) {
			if (0 >= balance(root->right)) {
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
iterate(struct node *root, ra_map_fnc_t fnc, void *ctx)
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

ra_map_t
ra_map_open(void)
{
	struct ra_map *map;

	if (!(map = malloc(sizeof (struct ra_map)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(map, 0, sizeof (struct ra_map));
	return map;
}

void
ra_map_close(ra_map_t map)
{
	if (map) {
		destroy(map->root);
		memset(map, 0, sizeof (struct ra_map));
		RA_FREE(map);
	}
}

void
ra_map_empty(ra_map_t map)
{
	if (map) {
		destroy(map->root);
		memset(map, 0, sizeof (struct ra_map));
	}
}

int
ra_map_update(ra_map_t map, const char *key, const void *val)
{
	struct node *root;

	assert( map && key && strlen(key) );

	if (!(root = update(map, map->root, key, val))) {
		RA_TRACE("^");
		return -1;
	}
	map->root = root;
	return 0;
}

void *
ra_map_lookup(ra_map_t map, const char *key)
{
	const struct node *node;
	int d;

	assert( map && key && strlen(key) );

	node = map->root;
	while (node) {
		if (!(d = strcmp(key, node->key))) {
			return (void *)node->val;
		}
		node = (0 > d) ? node->left : node->right;
	}
	return NULL;
}

int
ra_map_iterate(ra_map_t map, ra_map_fnc_t fnc, void *ctx)
{
	int e;

	assert( map && fnc );

	if ((e = iterate(map->root, fnc, ctx))) {
		return e;
	}
	return 0;
}

uint64_t
ra_map_items(ra_map_t map)
{
	assert( map );

	return map->items;
}

int
ra_map_test(void)
{
	const int N = 200000;
	const void *val_;
	ra_map_t map;
	char key[32];
	char val[32];
	int i;

	/* initialize */

	if (!(map = ra_map_open())) {
		RA_TRACE("^");
		return -1;
	}
	if (0 != ra_map_items(map)) {
		ra_map_close(map);
		RA_TRACE("integrity failure detected");
		return -1;
	}

	/* single item */

	if (ra_map_update(map, "key", "val") ||
	    (1 != ra_map_items(map)) ||
	    strcmp("val",
		   ra_map_lookup(map, "key") ?
		   ra_map_lookup(map, "key") : "") ||
	    ra_map_update(map, "key", "lav") ||
	    (1 != ra_map_items(map)) ||
	    strcmp("lav",
		   ra_map_lookup(map, "key") ?
		   ra_map_lookup(map, "key") : "")) {
		ra_map_close(map);
		RA_TRACE("integrity failure detected");
		return -1;
	}

	/* empty */

	ra_map_empty(map);
	if (0 != ra_map_items(map)) {
		ra_map_close(map);
		RA_TRACE("integrity failure detected");
		return -1;
	}

	/* random update */

	srand(10);
	for (i=0; i<N; ++i) {
		ra_sprintf(key, sizeof (key), "%d-%d", rand(), i);
		ra_sprintf(val, sizeof (val), "%d-%d", rand(), i);
		if (ra_map_update(map, key, val)) {
			ra_map_close(map);
			RA_TRACE("integrity failure detected");
			return -1;
		}
		val_ = ra_map_lookup(map, key);
		if (!val_ || strcmp(val_, val)) {
			ra_map_close(map);
			RA_TRACE("integrity failure detected");
			return -1;
		}
	}

	/* random lookup */

	srand(10);
	for (i=0; i<N; ++i) {
		ra_sprintf(key, sizeof (key), "%d-%d", rand(), i);
		ra_sprintf(val, sizeof (val), "%d-%d", rand(), i);
		val_ = ra_map_lookup(map, key);
		if (!val_ || strcmp(val_, val)) {
			ra_map_close(map);
			RA_TRACE("integrity failure detected");
			return -1;
		}
	}

	/* empty */

	ra_map_empty(map);
	if (0 != ra_map_items(map)) {
		ra_map_close(map);
		RA_TRACE("integrity failure detected");
		return -1;
	}

	/* done */

	ra_map_close(map);
	return 0;
}
