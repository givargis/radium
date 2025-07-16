/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_index_succinct.c
 */

#include "ra_index_succinct.h"

#define CHAR2INT(c) ( (int)((unsigned char)(c)) )

struct ra__index_succinct {
	char *keys;
	uint64_t size;
	uint64_t items;
	uint64_t *refs;
	struct bitmap {
		uint64_t size;
		uint64_t *memory;
		uint32_t *popcount;
	} nodes, valids;
};

static int
bitmap_create(struct bitmap *bitmap, uint64_t size)
{
	uint64_t n1, n2;

	memset(bitmap, 0, sizeof (struct bitmap));
	bitmap->size = RA__DUP(size, 64);
	n1 = bitmap->size * sizeof (bitmap->memory[0]);
	n2 = bitmap->size * sizeof (bitmap->popcount[0]);
	if (!(bitmap->memory = ra__malloc(n1)) ||
	    !(bitmap->popcount = ra__malloc(n2))) {
		RA__ERROR_TRACE(0);
		return -1;
	}
	memset(bitmap->memory, 0, n1);
	memset(bitmap->popcount, 0, n2);
	return 0;
}

static void
bitmap_destroy(struct bitmap *bitmap)
{
	RA__FREE(bitmap->memory);
	RA__FREE(bitmap->popcount);
	memset(bitmap, 0, sizeof (struct bitmap));
}

static void
bitmap_prepare(struct bitmap *bitmap)
{
	uint64_t n;

	n = 0;
	for (uint64_t i=0; i<bitmap->size; ++i) {
		n += ra__popcount(bitmap->memory[i]);
		bitmap->popcount[i] = (uint32_t)n;
	}
}

static uint64_t
bitmap_rank(const struct bitmap *bitmap, uint64_t i)
{
	union { uint64_t *p64; uint32_t *p32; uint16_t *p16; } u;
	uint64_t q = i / 64;
	uint64_t r = i % 64;
	uint64_t popcount;

	u.p64 = &bitmap->memory[q];
	popcount = (q ? bitmap->popcount[q - 1] : 0);
	if (32 <= r) {
		popcount += ra__popcount(*u.p32);
		u.p32 += 1;
		r -= 32;
	}
	if (16 <= r) {
		popcount += ra__popcount(*u.p16);
		u.p16 += 1;
		r -= 16;
	}
	for (uint64_t i=0; i<=r; ++i) {
		popcount += (*u.p16 & (1 << i)) ? 1 : 0;
	}
	return popcount;
}

static void
bitmap_set(const struct bitmap *bitmap, uint64_t i)
{
	const uint64_t Q = i / 64;
	const uint64_t R = i % 64;

	bitmap->memory[Q] |= (1LU << R);
}

static int
bitmap_get(const struct bitmap *bitmap, uint64_t i)
{
	const uint64_t Q = i / 64;
	const uint64_t R = i % 64;

	return (bitmap->memory[Q] & (1LU << R)) ? 1 : 0;
}

static uint64_t
get_node(const struct ra__index_succinct *succinct, uint64_t i)
{
	if (bitmap_get(&succinct->nodes, i)) {
		return (3 * bitmap_rank(&succinct->nodes, i));
	}
	return 0;
}

static uint64_t
find(const struct ra__index_succinct *succinct, const char *key)
{
	uint64_t root;
	int d;

	root = 3;
	while (root) {
		d = CHAR2INT(*key) - CHAR2INT(succinct->keys[root / 3]);
		if (!d) {
			if ('\0' == (*(++key))) {
				break;
			}
			root = get_node(succinct, root + 1);
		}
		else if (0 > d) {
			root = get_node(succinct, root + 0);
		}
		else {
			root = get_node(succinct, root + 2);
		}
	}
	if (root) {
		if (bitmap_get(&succinct->valids, root / 3)) {
			return bitmap_rank(&succinct->valids, root / 3);
		}
	}
	return 0;
}

static uint64_t
min(const struct ra__index_succinct *succinct, uint64_t root, char *okey)
{
	uint64_t i, node;

	i = 0;
	while (root) {
		if (!(node = get_node(succinct, root + 0))) {
			okey[i++] = succinct->keys[root / 3];
			okey[i] = '\0';
			if (bitmap_get(&succinct->valids, root / 3)) {
				return bitmap_rank(&succinct->valids, root/3);
			}
			node = get_node(succinct, root + 1);
		}
		root = node;
	}
	return 0;
}

static uint64_t
max(const struct ra__index_succinct *succinct, uint64_t root, char *okey)
{
	uint64_t i, node;

	i = 0;
	while (root) {
		if (!(node = get_node(succinct, root + 2))) {
			okey[i++] = succinct->keys[root / 3];
			okey[i] = '\0';
			if (bitmap_get(&succinct->valids, root / 3)) {
				return bitmap_rank(&succinct->valids, root/3);
			}
			node = get_node(succinct, root + 1);
		}
		root = node;
	}
	return 0;
}

static uint64_t
next(const struct ra__index_succinct *succinct, const char *key, char *okey)
{
	uint64_t i, up, node, root, hold;
	int d, flag;

	i = 0;
	up = 0;
	root = 3;
	hold = 0;
	flag = 0;
	while (root) {
		if (0 > (d = (int)(*key) - (int)succinct->keys[root / 3])) {
			if (bitmap_get(&succinct->valids, root / 3) ||
			    get_node(succinct, root + 1)) {
				up = root;
				hold = i;
				flag = 1;
			}
			else if ((node = get_node(succinct, root + 2))) {
				up = node;
				hold = i;
				flag = 0;
			}
			root = get_node(succinct, root + 0);
		}
		else if (!d) {
			if ((node = get_node(succinct, root + 2))) {
				up = node;
				hold = i;
				flag = 0;
			}
			root = get_node(succinct, root + 1);
			okey[i++] = (*key);
			okey[i] = '\0';
			if ('\0' == (*(++key))) {
				break;
			}
		}
		else {
			root = get_node(succinct, root + 2);
		}
	}
	if (root) {
		return min(succinct, root, okey + i);
	}
	if (!up) {
		return 0;
	}
	i = hold;
	if (flag) {
		if (bitmap_get(&succinct->valids, up / 3)) {
			okey[i++] = succinct->keys[up / 3];
			okey[i] = '\0';
			return bitmap_rank(&succinct->valids, up / 3);
		}
		if ((root = get_node(succinct, up + 1))) {
			okey[i++] = succinct->keys[up / 3];
			okey[i] = '\0';
			return min(succinct, root, okey + i);
		}
		if ((root = get_node(succinct, up + 2))) {
			return min(succinct, root, okey + i);
		}
	}
	else {
		return min(succinct, up, okey + i);
	}
	return 0;
}

static uint64_t
prev(const struct ra__index_succinct *succinct, const char *key, char *okey)
{
	uint64_t i, up, node, root, hold;
	int d, flag;

	i = 0;
	up = 0;
	root = 3;
	hold = 0;
	flag = 0;
	while (root) {
		if (0 < (d = (int)(*key) - (int)succinct->keys[root / 3])) {
			if (bitmap_get(&succinct->valids, root / 3) ||
			    get_node(succinct, root + 1)) {
				up = root;
				hold = i;
				flag = 1;
			}
			else if ((node = get_node(succinct, root + 0))) {
				up = node;
				hold = i;
				flag = 0;
			}
			root = get_node(succinct, root + 2);
		}
		else if (!d) {
			if ((node = get_node(succinct, root + 0))) {
				up = node;
				hold = i;
				flag = 0;
			}
			root = get_node(succinct, root + 1);
			okey[i++] = (*key);
			okey[i] = '\0';
			if ('\0' == (*(++key))) {
				break;
			}
		}
		else {
			root = get_node(succinct, root + 0);
		}
	}
	if (root) {
		return max(succinct, root, okey + i);
	}
	if (!up) {
		return 0;
	}
	i = hold;
	if (flag) {
		if (bitmap_get(&succinct->valids, up / 3)) {
			okey[i++] = succinct->keys[up / 3];
			okey[i] = '\0';
			return bitmap_rank(&succinct->valids, up / 3);
		}
		if ((root = get_node(succinct, up + 1))) {
			okey[i++] = succinct->keys[up / 3];
			okey[i] = '\0';
			return max(succinct, root, okey + i);
		}
		if ((root = get_node(succinct, up + 0))) {
			return max(succinct, root, okey + i);
		}
	}
	else {
		return max(succinct, up, okey + i);
	}
	return 0;
}

static void
_encode_(void *ctx, char key, int left, int center, int right,uint64_t *ref)
{
	struct ra__index_succinct *succinct;

	succinct = (struct ra__index_succinct *)ctx;
	if (left) {
		bitmap_set(&succinct->nodes, succinct->size * 3 + 0);
	}
	if (center) {
		bitmap_set(&succinct->nodes, succinct->size * 3 + 1);
	}
	if (right) {
		bitmap_set(&succinct->nodes, succinct->size * 3 + 2);
	}
	if (ref) {
		bitmap_set(&succinct->valids, succinct->size);
		succinct->refs[succinct->items++] = (*ref);
	}
	succinct->keys[succinct->size++] = key;
}

ra__index_succinct_t
ra__index_succinct_open(ra__index_ternary_t ternary)
{
	struct ra__index_succinct *succinct;
	uint64_t size, items, n1, n2;

	assert( ternary );

	if (!(succinct = ra__malloc(sizeof (struct ra__index_succinct)))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(succinct, 0, sizeof (struct ra__index_succinct));
	if (ra__index_ternary_items(ternary)) {
		size = ra__index_ternary_nodes(ternary) + 1;
		items = ra__index_ternary_items(ternary) + 1;
		if (bitmap_create(&succinct->nodes, size * 3) ||
		    bitmap_create(&succinct->valids, size * 1)) {
			ra__index_succinct_close(succinct);
			RA__ERROR_TRACE(0);
			return NULL;
		}
		n1 = size * sizeof (succinct->keys[0]);
		n2 = items * sizeof (succinct->refs[0]);
		if (!(succinct->keys = ra__malloc(n1)) ||
		    !(succinct->refs = ra__malloc(n2))) {
			ra__index_succinct_close(succinct);
			RA__ERROR_TRACE(0);
			return NULL;
		}
		memset(succinct->keys, 0, n1);
		memset(succinct->refs, 0, n2);
		succinct->size = 1;
		succinct->items = 1;
		bitmap_set(&succinct->nodes, 1);
		if (ra__index_ternary_iterate(ternary, _encode_, succinct)) {
			ra__index_succinct_close(succinct);
			RA__ERROR_TRACE(0);
			return NULL;
		}
		bitmap_prepare(&succinct->nodes);
		bitmap_prepare(&succinct->valids);
		assert( size == succinct->size );
		assert( items == succinct->items );
	}
	return succinct;
}

void
ra__index_succinct_close(ra__index_succinct_t succinct)
{
	if (succinct) {
		bitmap_destroy(&succinct->nodes);
		bitmap_destroy(&succinct->valids);
		RA__FREE(succinct->keys);
		RA__FREE(succinct->refs);
		memset(succinct, 0, sizeof (struct ra__index_succinct));
	}
	RA__FREE(succinct);
}

uint64_t *
ra__index_succinct_find(ra__index_succinct_t succinct, const char *key)
{
	uint64_t i;

	assert( succinct );
	assert( key );

	i = 0;
	if (succinct->items) {
		i = find(succinct, key);
	}
	return i ? &succinct->refs[i] : NULL;
}

uint64_t *
ra__index_succinct_next(ra__index_succinct_t succinct,
			const char *key,
			char *okey)
{
	uint64_t i;

	assert( succinct );
	assert( okey );

	i = 0;
	if (succinct->items) {
		if (ra__strlen(key)) {
			i = next(succinct, key, okey);
		}
		else {
			i = min(succinct, 3, okey);
		}
	}
	return i ? &succinct->refs[i] : NULL;
}

uint64_t *
ra__index_succinct_prev(ra__index_succinct_t succinct,
			const char *key,
			char *okey)
{
	uint64_t i;

	assert( succinct );
	assert( okey );

	i = 0;
	if (succinct->items) {
		if (ra__strlen(key)) {
			i = prev(succinct, key, okey);
		}
		else {
			i = max(succinct, 3, okey);
		}
	}
	return i ? &succinct->refs[i] : NULL;
}

uint64_t
ra__index_succinct_items(ra__index_succinct_t succinct)
{
	assert( succinct );

	return succinct->items ? (succinct->items - 1) : 0;
}
