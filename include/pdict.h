/**
 * pdict
 *
 * pdict.h
 *
 * Paulo Tobias
 * paulohtobias@outlook.com
 */

#ifndef PDICT_H
#define PDICT_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/// Memory allocators.
#ifndef __PDICT_ALLOC__
#define __PDICT_ALLOC__
#define pdict_malloc malloc
#define pdict_calloc calloc
#define pdict_realloc realloc
#define pdict_free free
#endif // __PDICT_ALLOC__

/// Default maximum capacity.
#ifndef __PDICT_DEFAULT_MAX_LEN__
#define __PDICT_DEFAULT_MAX_LEN__ 100 // Arbitrary value.
#endif // __PDICT_DEFAULT_MAX_LEN__

/// Default stretch increase length.
#ifndef __PDICT_DEFAULT_STRETCH__
#define __PDICT_DEFAULT_STRETCH__ 0
#endif // __PDICT_DEFAULT_STRETCH__

/// MurmurHash3's seed.
#ifndef __PDICT_HASH_SEED__
#define __PDICT_HASH_SEED__ 22 // Arbitrary value.
#endif // __PDICT_HASH_SEED__

/// Error codes.
enum {
	PDICT_OK,
	PDICT_E_FULL,
	PDICT_E_KEY_NOT_FOUND
};
extern _Thread_local int __pdict_errno;

/**
 * Describes a dict item.
 *
 * `key`: unique string identifier for `value`.
 *
 * `value`: pointer to some value that will be stored.
 *
 * `free_value`: function to call on `value` to release its memory.
 *               Can be NULL.
 *
 * `order`: increases for every new key added. Can be used
 *          to get a keys list sorted by insertion order.
 */
typedef struct pdict_item_t pdict_item_t;
struct pdict_item_t {
	char *key;
	void *value;

	void (*free_value)(void *);

	int32_t order;
};

/// Opaque dict struct.
typedef struct pdict_t pdict_t;

/**
 * Returns a string describing the last error.
 */
const char *pdict_get_last_error_message();

/**
 * Returns a string describing the given error.
 */
const char *pdict_get_error_message(int pdict_errno);

/**
 *
 */
pdict_t *pdict_create_all(int32_t max_len, bool add_missing_keys, int32_t stretch);

/**
 *
 */
pdict_t *pdict_create();

/**
 * Release `dict` from memory.
 */
void pdict_destroy(pdict_t *dict);

/**
 * Checks if `key` exists in `dict`.
 */
#define pdict_has_key(dict, key) (pdict_get_item(dict, key) != NULL)

/**
 * Returns a pointer to the value associated with `key` or `default_value`
 * if `key` doesn't exist.
 */
const void *pdict_get_value_default(const pdict_t *dict, const char *key, const void *default_value);

/**
 * Wrapper for `pdict_get_value_default` using `NULL` as the default value.
 */
#define pdict_get_value(dict, key) pdict_get_value_default(dict, key, NULL);

/**
 *
 */
pdict_item_t *pdict_get_item(const pdict_t *dict, const char *key);

/**
 * Set's the value associated with `key`.
 *
 * If the key doesn't exist and `add_missing_keys` is `true`,
 * the key will be automatically created. If `add_missing_keys`
 * is `false`, then no value is added.
 *
 */
int pdict_set_value_all(pdict_t *dict, const char *key, void *value, void (*free_value)(void *));

/**
 * Wrapper to `pdict_set_value_all` using `pdict_free` as
 * the default `free_value` function.
 */
#define pdict_set_value(dict, key, value) pdict_set_value_all(dict, key, value, pdict_free)

/**
 * Same as `pdict_set_value_all` but alwalys create missing keys.
 */
int pdict_add_value_all(pdict_t *dict, const char *key, void *value, void (*free_value)(void *));

/**
 * Wrapper to `pdict_add_value_all` using `pdict_free` as
 * the default `free_value` function.
 */
#define pdict_add_value(dict, key, value) pdict_add_value_all(dict, key, value, pdict_free)

/**
 * Return a list with all the keys from `dict`. All
 * strings will be allocated using `pdict_malloc`.
 *
 * If `sort` is `true`, then keys will be sorted using its `item`
 * `order` attribute, i.e., the insertion order.
 */
char **pdict_get_keys(const pdict_t *dict, int32_t *keys_len, bool sort);

#endif // PDICT_H
