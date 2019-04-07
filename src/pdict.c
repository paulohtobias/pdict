/**
 * Copyright (c) 2019 Paulo Tobias <paulohtobias@outlook.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pdict.h"
#include <string.h>

#define __PDICT_STRDUP(dest, src) {           \
		dest = pdict_malloc(strlen(src) + 1); \
		strcpy(dest, src);                    \
	}

typedef struct pdict_t {
	pdict_item_t *items;

	/// Current number of items.
	int32_t len;

	// Maximum capacity.
	int32_t max_len;

	/// If a non-existing key will be added
	/// when setting a value.
	/// Default: `true`.
	bool add_missing_keys;

	/// If > 0, `items` will stretch, i.e.,
	/// realloc'd when `len` reaches `max_len`.
	/// Default: `__PDICT_DEFAULT_STRETCH__`.
	int32_t stretch;
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

pdict_t *pdict_create_all(int32_t max_len, bool add_missing_keys, int32_t stretch) {
	pdict_t *dict = pdict_malloc(sizeof(pdict_t));

	dict->max_len = max_len;
	dict->items = pdict_calloc(dict->max_len, sizeof(pdict_item_t));
	dict->len = 0;
	dict->add_missing_keys = add_missing_keys;
	dict->stretch = stretch;

	return dict;
}

pdict_t *pdict_create() {
	return pdict_create_all(__PDICT_DEFAULT_MAX_LEN__, true, __PDICT_DEFAULT_STRETCH__);
}

void pdict_destroy(pdict_t *dict) {
	int32_t i, j;
	for(i = 0, j = 0; j < dict->len && i < dict->max_len; i++) {
		if (dict->items[i].key != NULL) {
			pdict_free(dict->items[i].key);
			if (dict->items[i].value != NULL) {
				dict->items[i].free_value(dict->items[i].value);
			}

			j++;
		}
	}

	pdict_free(dict->items);
	pdict_free(dict);
}

static inline int32_t __pdict_hash_abs(int32_t hash) {
	return hash > 0 ? hash : -hash;
}

static void MurmurHash3_x64_128(const void *key, int len, uint32_t seed, void *out);

static int32_t __pdict_get_items_index(const pdict_item_t *items, int32_t max_len, const char *key, bool find_empty) {
	__pdict_errno = PDICT_OK;

	int32_t i;
	int32_t index;
	int32_t hash[4];
	int32_t start = 0;

	// Hash the key.
	MurmurHash3_x64_128(key, strlen(key), __PDICT_HASH_SEED__, hash);

	// MurmurHash3_x64_128 will give four 32-bit values. try them first.
	for (i = 0; i < 4; i++) {
		index = __pdict_hash_abs(hash[i] % max_len);

		start += index;

		if ((items[index].key != NULL && !find_empty && strcmp(items[index].key, key) == 0) ||
			(items[index].key == NULL && find_empty))
		{
			return index;
		}
	}

	start = __pdict_hash_abs(start % max_len);

	// If even after trying all four we can't get a match, we start a naive linear search.
	for (i = 0; i < max_len; i++) {
		index = __pdict_hash_abs((i + start) % max_len);

		if ((items[index].key != NULL && !find_empty && strcmp(items[index].key, key) == 0) ||
			(items[index].key == NULL && find_empty))
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

static inline int32_t __pdict_get_index(const pdict_t *dict, const char *key, bool find_empty) {
	return __pdict_get_items_index(dict->items, dict->max_len, key, find_empty);
}

static void __pdict_stretch(pdict_t *dict) {
	if (dict->stretch < 1) {
		return;
	}

	int32_t max_len = dict->max_len + dict->stretch;
	pdict_item_t *items = pdict_calloc(max_len, sizeof(pdict_item_t));

	int32_t i, j;
	for (i = 0, j = 0; j < dict->len && i < dict->max_len; i++) {
		if (dict->items[i].key != NULL) { // Should be always true.

			// new index should always be greater than 0.
			int32_t index = __pdict_get_items_index(items, max_len, dict->items[i].key, true);

			items[index] = dict->items[i];

			j++;
		}
	}

	pdict_free(dict->items);
	dict->items = items;
	dict->max_len = max_len;
}

const void *pdict_get_value_default(const pdict_t *dict, const char *key, const void *default_value) {
	__pdict_errno = PDICT_OK;

	pdict_item_t *item = pdict_get_item(dict, key);
	return item != NULL ? item->value : default_value;
}

pdict_item_t *pdict_get_item(const pdict_t *dict, const char *key) {
	__pdict_errno = PDICT_OK;

	int32_t index = __pdict_get_index(dict, key, false);

	return index >= 0 ? &dict->items[index] : NULL;
}

static int __pdict_set_value(pdict_t *dict, const char *key, void *value, bool add_missing_keys, void (*free_value)(void *)) {
	pdict_item_t *item = pdict_get_item(dict, key);

	if (item == NULL) {
		if (add_missing_keys) {
			int32_t index = __pdict_get_index(dict, key, true);

			// Couldn't find an empty position to add the key.
			if (index < 0) {
				if (dict->stretch <= 0) {
					return 1;
				}

				__pdict_stretch(dict);
				index = __pdict_get_index(dict, key, true);
			}

			__pdict_errno = PDICT_OK;

			item = &dict->items[index];
			__PDICT_STRDUP(item->key, key);
			item->order = dict->len++;
		} else {
			return 1;
		}
	} else if (item->free_value != NULL) {
		item->free_value(item->value);
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

int32_t pdict_get_key_count(const pdict_t *dict) {
	return dict->len;
}

char **pdict_get_keys(const pdict_t *dict, int32_t *keys_len, bool sort) {
	char **keys = pdict_malloc(dict->len * sizeof(char *));
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

//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

// Note - The x86 and x64 versions do _not_ produce the same results, as the
// algorithms are optimized for their respective platforms. You can still
// compile and run any of them on any platform, but your performance with the
// non-native version will be less than optimal.

//-----------------------------------------------------------------------------
// Platform-specific functions and macros

#ifdef __GNUC__
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
#define FORCE_INLINE inline
#endif

static FORCE_INLINE uint32_t rotl32(uint32_t x, int8_t r) {
  return (x << r) | (x >> (32 - r));
}

static FORCE_INLINE uint64_t rotl64(uint64_t x, int8_t r ){
  return (x << r) | (x >> (64 - r));
}

#define	ROTL32(x,y)	rotl32(x,y)
#define ROTL64(x,y)	rotl64(x,y)

#define BIG_CONSTANT(x) (x##LLU)

//-----------------------------------------------------------------------------
// Block read - if your platform needs to do endian-swapping or can only
// handle aligned reads, do the conversion here

#define getblock(p, i) (p[i])

//-----------------------------------------------------------------------------
// Finalization mix - force all bits of a hash block to avalanche

static FORCE_INLINE uint32_t fmix32(uint32_t h) {
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;

  return h;
}

//----------

static FORCE_INLINE uint64_t fmix64(uint64_t k) {
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xff51afd7ed558ccd);
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
  k ^= k >> 33;

  return k;
}

//-----------------------------------------------------------------------------

static void MurmurHash3_x64_128(const void * key, const int len,
                           const uint32_t seed, void * out) {

	const uint8_t * data = (const uint8_t*)key;
	const int nblocks = len / 16;
	int i;

	uint64_t h1 = seed;
	uint64_t h2 = seed;

	uint64_t c1 = BIG_CONSTANT(0x87c37b91114253d5);
	uint64_t c2 = BIG_CONSTANT(0x4cf5ad432745937f);

	//----------
	// body

	const uint64_t * blocks = (const uint64_t *)(data);

	for(i = 0; i < nblocks; i++) {
		uint64_t k1 = getblock(blocks,i*2+0);
		uint64_t k2 = getblock(blocks,i*2+1);

		k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;

		h1 = ROTL64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

		k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;

		h2 = ROTL64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
	}

	//----------
	// tail

	const uint8_t * tail = (const uint8_t*)(data + nblocks*16);

	uint64_t k1 = 0;
	uint64_t k2 = 0;

	switch(len & 15) {
		case 15: k2 ^= (uint64_t)(tail[14]) << 48;
		case 14: k2 ^= (uint64_t)(tail[13]) << 40;
		case 13: k2 ^= (uint64_t)(tail[12]) << 32;
		case 12: k2 ^= (uint64_t)(tail[11]) << 24;
		case 11: k2 ^= (uint64_t)(tail[10]) << 16;
		case 10: k2 ^= (uint64_t)(tail[ 9]) << 8;
		case  9: k2 ^= (uint64_t)(tail[ 8]) << 0;
			     k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;

		case  8: k1 ^= (uint64_t)(tail[ 7]) << 56;
		case  7: k1 ^= (uint64_t)(tail[ 6]) << 48;
		case  6: k1 ^= (uint64_t)(tail[ 5]) << 40;
		case  5: k1 ^= (uint64_t)(tail[ 4]) << 32;
		case  4: k1 ^= (uint64_t)(tail[ 3]) << 24;
		case  3: k1 ^= (uint64_t)(tail[ 2]) << 16;
		case  2: k1 ^= (uint64_t)(tail[ 1]) << 8;
		case  1: k1 ^= (uint64_t)(tail[ 0]) << 0;
			     k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;
	};

	//----------
	// finalization

	h1 ^= len; h2 ^= len;

	h1 += h2;
	h2 += h1;

	h1 = fmix64(h1);
	h2 = fmix64(h2);

	h1 += h2;
	h2 += h1;

	((uint64_t*)out)[0] = h1;
	((uint64_t*)out)[1] = h2;
}

//-----------------------------------------------------------------------------
