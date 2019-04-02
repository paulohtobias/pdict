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
#define __pdict_malloc malloc
#define __pdict_calloc calloc
#define __pdict_realloc realloc
#define __pdict_free free
#endif // __PDICT_ALLOC__

/// Default maximum capacity.
#ifndef __PDICT_DEFAULT_MAX_LEN__
#define __PDICT_DEFAULT_MAX_LEN__ 100 // Arbitrary value.
#endif // __PDICT_DEFAULT_MAX_LEN__

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
 *
 */
typedef struct pdict_item_t {
	char *key;
	void *value;

	void (*free_value)(void *);

	int32_t order;
} pdict_item_t;

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
pdict_t *pdict_new_all(int32_t max_len, bool add_missing_keys, bool stretch);

/**
 *
 */
pdict_t *pdict_new();

/**
 * Release `dict` from memory.
 */
void pdict_free(pdict_t *dict);

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
 * Wrapper to `pdict_set_value_all` using `__pdict_free` as
 * the default `free_value` function.
 */
#define pdict_set_value(dict, key, value) pdict_set_value_all(dict, key, value, __pdict_free)

/**
 * Same as `pdict_set_value_all` but alwalys create missing keys.
 */
int pdict_add_value_all(pdict_t *dict, const char *key, void *value, void (*free_value)(void *));

/**
 * Wrapper to `pdict_add_value_all` using `__pdict_free` as
 * the default `free_value` function.
 */
#define pdict_add_value(dict, key, value) pdict_add_value_all(dict, key, value, __pdict_free)

/**
 * Return a list with all the keys from `dict`. All
 * strings will be allocated using `__pdict_malloc`.
 *
 * If `sort` is `true`, then keys will be sorted using its `item`
 * `order` attribute, i.e., the insertion order.
 */
char **pdict_get_keys(const pdict_t *dict, int32_t *keys_len, bool sort);

#endif // PDICT_H
