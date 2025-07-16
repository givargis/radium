/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_index.c
 */

#include "ra_index_succinct.h"
#include "ra_index.h"

#define TEST(m,e)						\
	do {							\
		if ((e)) {					\
			t = ra__time() - t;			\
			ra__term_color(RA__TERM_COLOR_RED);	\
			ra__term_bold();			\
			printf("\t [FAIL] ");			\
			ra__term_reset();			\
			printf("%20s %6.1fs\n", (m), 1e-6*t);   \
			t = ra__time();				\
		}						\
		else {						\
			t = ra__time() - t;			\
			ra__term_color(RA__TERM_COLOR_GREEN);	\
			ra__term_bold();			\
			printf("\t [PASS] ");			\
			ra__term_reset();			\
			printf("%20s %6.1fs\n", (m), 1e-6*t);   \
			t = ra__time();				\
		}						\
	} while (0)

struct ra__index {
	ra__index_tree_t tree;
	ra__index_succinct_t succinct;
};

ra__index_t
ra__index_open(void)
{
	struct ra__index *index;

	assert( RA__INDEX_MAX_KEY_LEN == RA__INDEX_TREE_MAX_KEY_LEN );

	if (!(index = ra__malloc(sizeof (struct ra__index)))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(index, 0, sizeof (struct ra__index));
	if (!(index->tree = ra__index_tree_open())) {
		ra__index_close(index);
		RA__ERROR_TRACE(0);
		return NULL;
	}
	return index;
}

void
ra__index_close(ra__index_t index)
{
	if (index) {
		ra__index_tree_close(index->tree);
		ra__index_succinct_close(index->succinct);
		memset(index, 0, sizeof (struct ra__index));
	}
	RA__FREE(index);
}

void
ra__index_truncate(ra__index_t index)
{
	assert( index );

	ra__index_tree_truncate(index->tree);
	ra__index_succinct_close(index->succinct);
	index->succinct = NULL;
}

int
ra__index_compress(ra__index_t index)
{
	ra__index_ternary_t ternary;

	assert( index );
	assert( !index->succinct );

	if (!(ternary = ra__index_ternary_open(index->tree))) {
		RA__ERROR_TRACE(0);
		return -1;
	}
	ra__index_tree_truncate(index->tree);
	if (!(index->succinct = ra__index_succinct_open(ternary))) {
		RA__ERROR_TRACE(0);
		return -1;
	}
	ra__index_ternary_close(ternary);
	return 0;
}

uint64_t *
ra__index_update(ra__index_t index, const char *key)
{
	uint64_t *ref;

	assert( index );
	assert( !index->succinct );
	assert( ra__strlen(key) );
	assert( RA__INDEX_MAX_KEY_LEN > ra__strlen(key) );

	if (!(ref = ra__index_tree_update(index->tree, key))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	return ref;
}

uint64_t *
ra__index_find(ra__index_t index, const char *key)
{
	assert( index );
	assert( ra__strlen(key) );

	if (index->succinct) {
		return ra__index_succinct_find(index->succinct, key);
	}
	return ra__index_tree_find(index->tree, key);
}

uint64_t *
ra__index_next(ra__index_t index, const char *key, char *okey)
{
	assert( index );
	assert( okey );

	if (index->succinct) {
		return ra__index_succinct_next(index->succinct, key, okey);
	}
	return ra__index_tree_next(index->tree, key, okey);
}

uint64_t *
ra__index_prev(ra__index_t index, const char *key, char *okey)
{
	assert( index );
	assert( okey );

	if (index->succinct) {
		return ra__index_succinct_prev(index->succinct, key, okey);
	}
	return ra__index_tree_prev(index->tree, key, okey);
}

uint64_t
ra__index_items(ra__index_t index)
{
	assert( index );

	if (index->succinct) {
		return ra__index_succinct_items(index->succinct);
	}
	return ra__index_tree_items(index->tree);
}

int
ra__index_bist(void)
{
	const int N = 1234567;
	ra__index_t index;
	uint64_t *ref;
	char okey[64];
	char key[64];
	uint64_t t;
	int j;

	// initialize

	t = ra__time();
	if (!(index = ra__index_open())) {
		RA__ERROR_TRACE(0);
		return -1;
	}

	// zero item logic

	ra__index_truncate(index);
	if ((0 != ra__index_items(index)) ||
	    (NULL != ra__index_find(index, "K")) ||
	    (NULL != ra__index_next(index, "K", okey)) ||
	    (NULL != ra__index_next(index, NULL, okey)) ||
	    (NULL != ra__index_prev(index, "K", okey)) ||
	    (NULL != ra__index_prev(index, NULL, okey)) ||
	    ra__index_compress(index) ||
	    (0 != ra__index_items(index)) ||
	    (NULL != ra__index_find(index, "K")) ||
	    (NULL != ra__index_find(index, "K")) ||
	    (NULL != ra__index_next(index, "K", okey)) ||
	    (NULL != ra__index_next(index, NULL, okey)) ||
	    (NULL != ra__index_prev(index, "K", okey)) ||
	    (NULL != ra__index_prev(index, NULL, okey))) {
		ra__index_close(index);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		TEST("zero-item-logic", -1);
		return -1;
	}
	TEST("zero-item-logic", 0);

	// single item logic

	ra__index_truncate(index);
	if ((0 != ra__index_items(index)) ||
	    !(ref = ra__index_update(index, "B")) ||
	    (1 != ra__index_items(index))) {
		ra__index_close(index);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		TEST("single-item-logic", -1);
		return -1;
	}
	(*ref) = 123;
	if ((NULL != ra__index_find(index, "A")) ||
	    (NULL != ra__index_next(index, "B", okey)) ||
	    (NULL != ra__index_prev(index, "A", okey)) ||
	    !(ref = ra__index_find(index, "B")) ||
	    (123 != (*ref)) ||
	    (ref != ra__index_next(index, NULL, okey)) ||
	    (123 != (*ref)) || strcmp("B", okey) ||
	    (ref != ra__index_prev(index, NULL, okey)) ||
	    (123 != (*ref)) || strcmp("B", okey) ||
	    (ref != ra__index_next(index, "", okey)) ||
	    (123 != (*ref)) || strcmp("B", okey) ||
	    (ref != ra__index_prev(index, "", okey)) ||
	    (123 != (*ref)) || strcmp("B", okey) ||
	    (ref != ra__index_next(index, "A", okey)) ||
	    (123 != (*ref)) || strcmp("B", okey) ||
	    (ref != ra__index_prev(index, "C", okey)) ||
	    (123 != (*ref)) || strcmp("B", okey) ||
	    ra__index_compress(index) ||
	    (1 != ra__index_items(index)) ||
	    (NULL != ra__index_find(index, "A")) ||
	    (NULL != ra__index_next(index, "B", okey)) ||
	    (NULL != ra__index_prev(index, "A", okey)) ||
	    !(ref = ra__index_find(index, "B")) ||
	    (123 != (*ref)) ||
	    (ref != ra__index_next(index, NULL, okey)) ||
	    (123 != (*ref)) || strcmp("B", okey) ||
	    (ref != ra__index_prev(index, NULL, okey)) ||
	    (123 != (*ref)) || strcmp("B", okey) ||
	    (ref != ra__index_next(index, "", okey)) ||
	    (123 != (*ref)) || strcmp("B", okey) ||
	    (ref != ra__index_prev(index, "", okey)) ||
	    (123 != (*ref)) || strcmp("B", okey) ||
	    (ref != ra__index_next(index, "A", okey)) ||
	    (123 != (*ref)) || strcmp("B", okey) ||
	    (ref != ra__index_prev(index, "C", okey)) ||
	    (123 != (*ref)) || strcmp("B", okey)) {
		ra__index_close(index);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		TEST("single-item-logic", -1);
		return -1;
	}
	TEST("single-item-logic", 0);

	// two item logic

	ra__index_truncate(index);
	if ((0 != ra__index_items(index)) ||
	    !(ref = ra__index_update(index, "A")) ||
	    (1 != ra__index_items(index))) {
		ra__index_close(index);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		TEST("two-item-logic", -1);
		return -1;
	}
	(*ref) = 123;
	if ((1 != ra__index_items(index)) ||
	    !(ref = ra__index_update(index, "C")) ||
	    (2 != ra__index_items(index))) {
		ra__index_close(index);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		TEST("two-item-logic", -1);
		return -1;
	}
	(*ref) = 321;
	if (!(ref = ra__index_find(index, "A")) ||
	    (123 != (*ref)) ||
	    !(ref = ra__index_find(index, "C")) ||
	    (321 != (*ref)) ||
	    (NULL != ra__index_find(index, "B")) ||
	    ra__index_compress(index) ||
	    (2 != ra__index_items(index)) ||
	    !(ref = ra__index_next(index, NULL, okey)) ||
	    (123 != (*ref)) || strcmp("A", okey) ||
	    !(ref = ra__index_prev(index, NULL, okey)) ||
	    (321 != (*ref)) || strcmp("C", okey) ||
	    !(ref = ra__index_next(index, "", okey)) ||
	    (123 != (*ref)) || strcmp("A", okey) ||
	    !(ref = ra__index_prev(index, "", okey)) ||
	    (321 != (*ref)) || strcmp("C", okey) ||
	    !(ref = ra__index_next(index, "B", okey)) ||
	    (321 != (*ref)) || strcmp("C", okey) ||
	    !(ref = ra__index_prev(index, "B", okey)) ||
	    (123 != (*ref)) || strcmp("A", okey) ||
	    (NULL != ra__index_find(index, "B"))) {
		ra__index_close(index);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		TEST("two-item-logic", -1);
		return -1;
	}
	TEST("two-item-logic", 0);

	// sequential update

	ra__index_truncate(index);
	for (int i=0; i<N; ++i) {
		ra__sprintf(key, sizeof (key), "k:%012d", i);
		if (!(ref = ra__index_update(index, key)) ||
		    !((*ref) = (i + 1)) ||
		    !(ref = ra__index_find(index, key)) ||
		    ((i + 1) != (int)(*ref)) ||
		    ((i + 1) != (int)ra__index_items(index))) {
			ra__index_close(index);
			RA__ERROR_TRACE(0);
			TEST("sequential-update", -1);
			return -1;
		}
	}
	TEST("sequential-update", 0);

	// sequential find

	for (int i=0; i<N; ++i) {
		ra__sprintf(key, sizeof (key), "k:%012d", i);
		if (!(ref = ra__index_find(index, key)) ||
		    ((i + 1) != (int)(*ref))) {
			ra__index_close(index);
			RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
			TEST("sequential-find", -1);
			break;
		}
	}
	TEST("sequential-find", 0);

	// random find

	for (int i=0; i<N; ++i) {
		j = rand() % N;
		ra__sprintf(key, sizeof (key), "k:%012d", j);
		if (!(ref = ra__index_find(index, key)) ||
		    ((j + 1) != (int)(*ref))) {
			ra__index_close(index);
			RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
			TEST("random-find", -1);
			break;
		}
	}
	TEST("random-find", 0);

	// compress

	if (ra__index_compress(index) || (N != ra__index_items(index))) {
		ra__index_close(index);
		RA__ERROR_TRACE(0);
		TEST("compress", -1);
		return -1;
	}
	TEST("compress", 0);

	// sequential find

	for (int i=0; i<N; ++i) {
		ra__sprintf(key, sizeof (key), "k:%012d", i);
		if (!(ref = ra__index_find(index, key)) ||
		    ((i + 1) != (int)(*ref))) {
			ra__index_close(index);
			RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
			TEST("sequential-find", -1);
			break;
		}
	}
	TEST("sequential-find", 0);

	// random find

	for (int i=0; i<N; ++i) {
		j = rand() % N;
		ra__sprintf(key, sizeof (key), "k:%012d", j);
		if (!(ref = ra__index_find(index, key)) ||
		    ((j + 1) != (int)(*ref))) {
			ra__index_close(index);
			RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
			TEST("random-find", -1);
			break;
		}
	}
	TEST("random-find", 0);

	// next find

	j = 0;
	key[0] = '\0';
	while ((ref = ra__index_next(index, key, okey))) {
		ra__sprintf(key, sizeof (key), "k:%012d", j);
		if (((j + 1) != (int)(*ref)) || strcmp(key, okey)) {
			ra__index_close(index);
			RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
			TEST("next-find", -1);
			return -1;
		}
		ra__sprintf(key, sizeof (key), "%s", okey);
		++j;
	}
	TEST("next-find", 0);

	// prev find

	j = N - 1;
	key[0] = '\0';
	while ((ref = ra__index_prev(index, key, okey))) {
		ra__sprintf(key, sizeof (key), "k:%012d", j);
		if (((j + 1) != (int)(*ref)) || strcmp(key, okey)) {
			ra__index_close(index);
			RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
			TEST("prev-find", -1);
			return -1;
		}
		ra__sprintf(key, sizeof (key), "%s", okey);
		--j;
	}
	TEST("prev-find", 0);

	// done

	ra__index_close(index);
	return 0;
}
