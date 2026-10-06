#include "ds_hashmap.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define HM_MIN_BUCKETS 8

/* Grow when size / bucket_count would exceed MAX_LOAD_NUM / MAX_LOAD_DEN (0.75). */
#define HM_MAX_LOAD_NUM 3
#define HM_MAX_LOAD_DEN 4

/*
 * One allocation per entry:  [ next | hash | value bytes | key chars + '\0' ]
 * The value comes first so it is aligned for any type.
 */
typedef struct hm_entry {
    struct hm_entry *next;  /* next entry in the same bucket (chaining)    */
    uint32_t         hash;  /* cached so resizing never re-hashes the key  */
    max_align_t      value[];
} hm_entry;

struct ds_hashmap {
    hm_entry **buckets;
    size_t     bucket_count; /* always a power of two */
    size_t     size;
    size_t     val_size;
};

static char *entry_key(const ds_hashmap *m, hm_entry *e)
{
    return (char *)e->value + m->val_size;
}

/* With a power-of-two table, `hash % count` is just a bit mask. */
static size_t bucket_of(const ds_hashmap *m, uint32_t hash)
{
    return hash & (m->bucket_count - 1);
}

uint32_t ds_hash_str(const char *s)
{
    uint32_t h = 2166136261u;          /* FNV offset basis */

    while (*s) {
        h ^= (unsigned char)*s++;
        h *= 16777619u;                /* FNV prime */
    }
    return h;
}

/*
 * Return the link (pointer-to-pointer) that points at key's entry, or at the
 * NULL that ends its chain. Insert and remove then need no special case for
 * the first entry in a bucket.
 */
static hm_entry **find_link(const ds_hashmap *m, const char *key, uint32_t hash)
{
    hm_entry **link = &m->buckets[bucket_of(m, hash)];

    while (*link && ((*link)->hash != hash || strcmp(entry_key(m, *link), key) != 0))
        link = &(*link)->next;
    return link;
}

/* Move every entry into a new table. Entries are relinked, not copied. */
static int rehash(ds_hashmap *m, size_t new_count)
{
    hm_entry **old = m->buckets;
    size_t old_count = m->bucket_count;
    hm_entry *e, *next;
    size_t i;

    m->buckets = calloc(new_count, sizeof *m->buckets);
    if (!m->buckets) {
        m->buckets = old;
        return DS_ERR_NOMEM;
    }
    m->bucket_count = new_count;

    for (i = 0; i < old_count; i++) {
        for (e = old[i]; e; e = next) {
            size_t b = bucket_of(m, e->hash);
            next = e->next;
            e->next = m->buckets[b];
            m->buckets[b] = e;
        }
    }
    free(old);
    return DS_OK;
}

ds_hashmap *hm_create(size_t val_size, size_t expected_entries)
{
    ds_hashmap *m;
    size_t count = HM_MIN_BUCKETS;

    if (expected_entries > SIZE_MAX / 8)
        return NULL;
    /* Smallest power of two that holds expected_entries below max load. */
    while (count * HM_MAX_LOAD_NUM < expected_entries * HM_MAX_LOAD_DEN)
        count *= 2;

    m = calloc(1, sizeof *m);
    if (!m)
        return NULL;
    m->buckets = calloc(count, sizeof *m->buckets);
    if (!m->buckets) {
        free(m);
        return NULL;
    }
    m->bucket_count = count;
    m->val_size = val_size;
    return m;
}

void hm_destroy(ds_hashmap *m)
{
    if (!m)
        return;
    hm_clear(m);
    free(m->buckets);
    free(m);
}

void hm_clear(ds_hashmap *m)
{
    hm_entry *e, *next;
    size_t i;

    if (!m)
        return;
    for (i = 0; i < m->bucket_count; i++) {
        for (e = m->buckets[i]; e; e = next) {
            next = e->next;
            free(e);
        }
        m->buckets[i] = NULL;
    }
    m->size = 0;
}

size_t hm_size(const ds_hashmap *m) { return m ? m->size : 0; }

int hm_put(ds_hashmap *m, const char *key, const void *value)
{
    hm_entry **link, *e;
    uint32_t hash;
    size_t key_len;

    if (!m || !key || (m->val_size && !value))
        return DS_ERR_INVALID;

    hash = ds_hash_str(key);
    link = find_link(m, key, hash);
    if (*link) { /* existing key: overwrite the value */
        memmove((*link)->value, value, m->val_size);
        return DS_OK;
    }

    if ((m->size + 1) * HM_MAX_LOAD_DEN > m->bucket_count * HM_MAX_LOAD_NUM
        && m->bucket_count <= SIZE_MAX / 2 / sizeof *m->buckets) {
        /* A failed resize is not fatal: the map just runs at a higher load. */
        if (rehash(m, m->bucket_count * 2) == DS_OK)
            link = find_link(m, key, hash);
    }

    key_len = strlen(key);
    e = malloc(sizeof *e + m->val_size + key_len + 1);
    if (!e)
        return DS_ERR_NOMEM;
    e->next = NULL;
    e->hash = hash;
    if (m->val_size)
        memcpy(e->value, value, m->val_size);
    memcpy(entry_key(m, e), key, key_len + 1);

    *link = e; /* link is the NULL at the end of the chain */
    m->size++;
    return DS_OK;
}

void *hm_get(const ds_hashmap *m, const char *key)
{
    hm_entry *e;

    if (!m || !key)
        return NULL;
    e = *find_link(m, key, ds_hash_str(key));
    return e ? (void *)e->value : NULL;
}

int hm_contains(const ds_hashmap *m, const char *key)
{
    return hm_get(m, key) != NULL;
}

int hm_remove(ds_hashmap *m, const char *key, void *out)
{
    hm_entry **link, *e;

    if (!m || !key)
        return DS_ERR_INVALID;
    link = find_link(m, key, ds_hash_str(key));
    e = *link;
    if (!e)
        return DS_ERR_NOTFOUND;

    if (out && m->val_size)
        memcpy(out, e->value, m->val_size);
    *link = e->next; /* unlink: works the same for the first or a middle entry */
    free(e);
    m->size--;
    return DS_OK;
}

void hm_iter_init(const ds_hashmap *m, ds_hashmap_iter *it)
{
    if (!it)
        return;
    it->map = m;
    it->bucket = 0;
    it->next_entry = NULL;
}

int hm_iter_next(ds_hashmap_iter *it, const char **key, void **value)
{
    const ds_hashmap *m;
    hm_entry *e;

    if (!it || !it->map)
        return 0;
    m = it->map;
    e = it->next_entry;
    while (!e && it->bucket < m->bucket_count)
        e = m->buckets[it->bucket++];
    if (!e)
        return 0;

    it->next_entry = e->next; /* saved now, so removing `e` is safe */
    if (key)
        *key = entry_key(m, e);
    if (value)
        *value = e->value;
    return 1;
}

size_t hm_bucket_count(const ds_hashmap *m) { return m ? m->bucket_count : 0; }

double hm_load_factor(const ds_hashmap *m)
{
    return m ? (double)m->size / (double)m->bucket_count : 0.0;
}

size_t hm_longest_chain(const ds_hashmap *m)
{
    size_t i, len, longest = 0;
    hm_entry *e;

    if (!m)
        return 0;
    for (i = 0; i < m->bucket_count; i++) {
        for (len = 0, e = m->buckets[i]; e; e = e->next)
            len++;
        if (len > longest)
            longest = len;
    }
    return longest;
}
