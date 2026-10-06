/*
 * ds_hashmap - hash map from strings to fixed-size values.
 *
 * Separate chaining, FNV-1a hashing, power-of-two bucket count.
 * The table doubles when the load factor would exceed 0.75, so
 * put/get/remove are O(1) on average.
 *
 * Keys and values are copied into the map. With val_size == 0 the map
 * works as a hash set (pass NULL as the value).
 */
#ifndef DS_HASHMAP_H
#define DS_HASHMAP_H

#include <stddef.h>
#include <stdint.h>
#include "ds_common.h"

typedef struct ds_hashmap ds_hashmap;

typedef struct {
    const ds_hashmap *map;
    size_t bucket;      /* next bucket to scan           */
    void  *next_entry;  /* next entry in current chain   */
} ds_hashmap_iter;

ds_hashmap *hm_create(size_t val_size, size_t expected_entries);
void        hm_destroy(ds_hashmap *m);

size_t hm_size(const ds_hashmap *m);

int   hm_put(ds_hashmap *m, const char *key, const void *value); /* insert or overwrite */
void *hm_get(const ds_hashmap *m, const char *key);             /* NULL if absent      */
int   hm_contains(const ds_hashmap *m, const char *key);
int   hm_remove(ds_hashmap *m, const char *key, void *out);     /* out may be NULL     */
void  hm_clear(ds_hashmap *m);

/*
 * Iteration in unspecified order. Removing the entry that was just returned
 * is safe; any other modification invalidates the iterator.
 */
void hm_iter_init(const ds_hashmap *m, ds_hashmap_iter *it);
int  hm_iter_next(ds_hashmap_iter *it, const char **key, void **value);

/* Introspection */
size_t hm_bucket_count(const ds_hashmap *m);
double hm_load_factor(const ds_hashmap *m);
size_t hm_longest_chain(const ds_hashmap *m);

uint32_t ds_hash_str(const char *s); /* 32-bit FNV-1a */

#endif /* DS_HASHMAP_H */
