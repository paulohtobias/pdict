/**
 * pdict
 *
 * pdict.c
 *
 * Paulo Tobias
 * paulohtobias@outlook.com
 */

#include "pdict.h"
#include "murmur3.h"
#include <string.h>

#define __PDICT_STRDUP(dest, src) \
	{                                           \
		dest = __pdict_malloc(strlen(src) + 1); \
		strcpy(dest, src);                      \
	}

typedef struct pdict_t {
	pdict_item_t *items;

	/// Current number of items.
	int32_t len;

	// Maximum capacity.
	int32_t max_len;

	/// If a non-existing key will be added
	/// when setting a value. Default is true.
	bool add_missing_keys;

	/// If `items` will stretch i.e. realloc
	/// when `len` reaches `max_len`.
	/// Not supported yet.
	bool stretch;
} pdict_t;

_Thread_local int __pdict_errno = PDICT_OK;
char __pdict_error_messages[][128] = {
	"No Error",
	"Dictionary is full",
	"Key not found"
};

const char *pdict_get_last_error_message() {
	return __pdict_error_messages[__pdict_errno];
}

const char *pdict_get_error_message(int pdict_errno) {
	return __pdict_error_messages[pdict_errno];
}

pdict_t *pdict_new_all(int32_t max_len, bool add_missing_keys, bool stretch) {
	pdict_t *dict = __pdict_malloc(sizeof(pdict_t));

	dict->max_len = max_len;
	dict->items = __pdict_calloc(dict->max_len, sizeof(pdict_item_t));
	dict->len = 0;
	dict->add_missing_keys = add_missing_keys;
	dict->stretch = stretch;

	return dict;
}

pdict_t *pdict_new() {
	return pdict_new_all(__PDICT_DEFAULT_MAX_LEN__, true, false);
}

void pdict_free(pdict_t *dict) {
	int32_t i, j;
	for(i = 0, j = 0; j < dict->len && i < dict->max_len; i++) {
		if (dict->items[i].key != NULL) {
			__pdict_free(dict->items[i].key);
			if (dict->items[i].value != NULL) {
				dict->items[i].free_value(dict->items[i].value);
			}

			j++;
		}
	}

	__pdict_free(dict->items);
	__pdict_free(dict);
}

static inline int32_t __pdict_hash_abs(int32_t hash) {
	return hash > 0 ? hash : -hash;
}

static int32_t __pdict_get_index(const pdict_t *dict, const char *key, bool find_empty) {
	__pdict_errno = PDICT_OK;

	int32_t i;
	int32_t index;
	int32_t hash[4];
	int32_t start = 0;

	// Hash the key.
	MurmurHash3_x64_128(key, strlen(key), __PDICT_HASH_SEED__, hash);

	// MurmurHash3_x64_128 will give four 32-bit values. try them first.
	for (i = 0; i < 4; i++) {
		index = __pdict_hash_abs(hash[i] % dict->max_len);

		start += index;

		if ((dict->items[index].key != NULL && !find_empty && strcmp(dict->items[index].key, key) == 0) ||
			(dict->items[index].key == NULL && find_empty))
		{
			return index;
		}
	}

	start = __pdict_hash_abs(start % dict->max_len);

	// If even after trying all four we can't get a match, we start a naive linear search.
	for (i = 0; i < dict->max_len; i++) {
		index = __pdict_hash_abs((i + start) % dict->max_len);

		if ((dict->items[index].key != NULL && !find_empty && strcmp(dict->items[index].key, key) == 0) ||
			(dict->items[index].key == NULL && find_empty))
		{
			return index;
		}
	}

	if (find_empty) {
		__pdict_errno = PDICT_E_FULL;
	} else {
		__pdict_errno = PDICT_E_KEY_NOT_FOUND;
	}

	return -1;
}

const void *pdict_get_value_default(const pdict_t *dict, const char *key, const void *default_value) {
	__pdict_errno = PDICT_OK;

	pdict_item_t *item = pdict_get_item(dict, key);
	return item != NULL ? item->value : default_value;
}

pdict_item_t *pdict_get_item(const pdict_t *dict, const char *key) {
	__pdict_errno = PDICT_OK;

	int32_t index = __pdict_get_index(dict, key, false);

	return index > 0 ? &dict->items[index] : NULL;
}

static int __pdict_set_value(pdict_t *dict, const char *key, void *value, bool add_missing_keys, void (*free_value)(void *)) {
	pdict_item_t *item = pdict_get_item(dict, key);

	if (item == NULL) {
		if (add_missing_keys) {
			int32_t index = __pdict_get_index(dict, key, true);
			if (index < 0) {
				return 1;
			}

			__pdict_errno = PDICT_OK;

			item = &dict->items[index];
			__PDICT_STRDUP(item->key, key);
			item->order = dict->len++;
		} else {
			return 1;
		}
	}

	item->value = value;
	item->free_value = free_value;

	return PDICT_OK;
}

int pdict_set_value_all(pdict_t *dict, const char *key, void *value, void (*free_value)(void *)) {
	return __pdict_set_value(dict, key, value, dict->add_missing_keys, free_value);
}

int pdict_add_value_all(pdict_t *dict, const char *key, void *value, void (*free_value)(void *)) {
	return __pdict_set_value(dict, key, value, true, free_value);
}

char **pdict_get_keys(const pdict_t *dict, int32_t *keys_len, bool sort) {
	char **keys = __pdict_malloc(dict->len * sizeof(char *));
	int32_t i, j, k;

	for (i = 0, j = 0; j < dict->len && i < dict->max_len; i++) {
		if (dict->items[i].key != NULL) {
			k = sort ? dict->items[i].order : j;

			__PDICT_STRDUP(keys[k], dict->items[i].key);

			j++;
		}
	}

	*keys_len = dict->len;

	return keys;
}

#undef __PDICT_STRDUP
