/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_index.h
 */

#ifndef _RA_INDEX_H_
#define _RA_INDEX_H_

#include "../utils/ra_utils.h"

#define RA__INDEX_MAX_KEY_LEN 32767

typedef struct ra__index *ra__index_t;

/**
 * Opens an empty index and returns an ra__index_t handle for subsequent use.
 *
 * @return  An ra__index_t handle or NULL on error
 */

ra__index_t ra__index_open(void);

/**
 * Closes the index and frees resources associated with it.
 *
 * @index  A valid index handle or NULL
 */

void ra__index_close(ra__index_t index);

/**
 * Removes all indexed items and resets the index to initial state.
 *
 * @index  A valid index handle
 */

void ra__index_truncate(ra__index_t index);

/**
 * Compresses the index, reducing its memory footprint.
 *
 * @index   A valid index handle
 * @return  0 on success or -1 on error
 *
 * NOTES: A compressed index is no longer able to accept new keys, effectively
 *        turning into a read-only dictionary. However, refs associated with
 *        existing keys can still be modified.
 */

int ra__index_compress(ra__index_t index);

/**
 * Updates the index by adding a new key or returning the ref associated with
 * an existing key.
 *
 * @index   A valid index handle
 * @key     A non-empty key
 * @return  A pointer to a ref, which can be modified by the caller, or NULL in
 *          case of an error
 */

uint64_t *ra__index_update(ra__index_t index, const char *key);

/**
 * Finds and returns the ref associated with the key.
 *
 * @index   A valid index handle
 * @key     A non-empty key
 * @return  A pointer to a ref, which can be modified by the caller, or NULL if
 *          the key does not exist
 */

uint64_t *ra__index_find(ra__index_t index, const char *key);

/**
 * Finds and returns the ref associated with the lexicographical
 * successor of key.
 *
 * @index   A valid index handle
 * @key     A non-empty key, or NULL
 * @return  A pointer to a ref, which can be modified by the caller, or NULL if
 *          the key does not exist
 *
 * NOTES: If the key is NULL, the function returns the ref associated with the
 *        smallest key. The key doesn't need to be present in the index,
 *        enabling a neighborhood search feature.
 */

uint64_t *ra__index_next(ra__index_t succinct, const char *key, char *okey);

/**
 * Finds and returns the ref associated with the lexicographical predecessor
 * of the key.
 *
 * @index   A valid index handle
 * @key     A non-empty key, or NULL
 * @return  A pointer to a ref, which can be modified by the caller, or NULL if
 *          the key does not exist
 *
 * NOTES: If the key is NULL, the function returns the ref associated with the
 *        smallest key. The key doesn't need to be present in the index,
 *        enabling a neighborhood search feature.
 */

uint64_t *ra__index_prev(ra__index_t succinct, const char *key, char *okey);

/**
 * Returns the number of indexed items.
 *
 * @index   A valid index handle
 * @return  The number of indexed items
 */

uint64_t ra__index_items(ra__index_t index);

/**
 * Runs the built-in self test.
 *
 * @return  0 on success or -1 on error
 */

int ra__index_bist(void);

#endif // _RA_INDEX_H_
