#include "ds_vector.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define VEC_MIN_CAPACITY 8

struct ds_vector {
    unsigned char *data;      /* capacity * elem_size bytes */
    size_t         size;      /* elements in use            */
    size_t         capacity;  /* elements allocated         */
    size_t         elem_size;
};

static unsigned char *slot(const ds_vector *v, size_t index)
{
    return v->data + index * v->elem_size;
}

static int set_capacity(ds_vector *v, size_t capacity)
{
    unsigned char *data;

    if (capacity == 0) {
        free(v->data);
        v->data = NULL;
        v->capacity = 0;
        return DS_OK;
    }
    if (capacity > SIZE_MAX / v->elem_size) /* capacity * elem_size overflows */
        return DS_ERR_NOMEM;

    data = realloc(v->data, capacity * v->elem_size);
    if (!data)
        return DS_ERR_NOMEM; /* the old buffer is still valid */
    v->data = data;
    v->capacity = capacity;
    return DS_OK;
}

/*
 * Make room for one more element. Doubling the capacity (instead of adding a
 * constant) is what makes push_back amortized O(1): n pushes cost at most
 * n + n/2 + n/4 + ... < 2n element copies in total.
 *
 * `elem` may point into our own buffer (e.g. push_back(v, vec_at(v, 0))).
 * realloc can move the buffer, so such a pointer is rebased afterwards.
 */
static int ensure_room(ds_vector *v, const void **elem)
{
    uintptr_t p = (uintptr_t)*elem;
    uintptr_t lo = (uintptr_t)v->data;
    uintptr_t hi = lo + v->size * v->elem_size;
    int aliased = v->data != NULL && p >= lo && p < hi;
    size_t cap;
    int rc;

    if (v->size < v->capacity)
        return DS_OK;

    cap = v->capacity ? v->capacity : VEC_MIN_CAPACITY;
    while (cap <= v->size) {
        if (cap > SIZE_MAX / 2)
            return DS_ERR_NOMEM;
        cap *= 2;
    }

    rc = set_capacity(v, cap);
    if (rc == DS_OK && aliased)
        *elem = v->data + (p - lo);
    return rc;
}

ds_vector *vec_create(size_t elem_size, size_t initial_capacity)
{
    ds_vector *v;

    if (elem_size == 0)
        return NULL;
    v = calloc(1, sizeof *v);
    if (!v)
        return NULL;
    v->elem_size = elem_size;
    if (initial_capacity && set_capacity(v, initial_capacity) != DS_OK) {
        free(v);
        return NULL;
    }
    return v;
}

void vec_destroy(ds_vector *v)
{
    if (!v)
        return;
    free(v->data);
    free(v);
}

size_t vec_size(const ds_vector *v) { return v ? v->size : 0; }
size_t vec_capacity(const ds_vector *v) { return v ? v->capacity : 0; }
int vec_empty(const ds_vector *v) { return vec_size(v) == 0; }
void *vec_data(const ds_vector *v) { return v ? v->data : NULL; }

void *vec_at(const ds_vector *v, size_t index)
{
    if (!v || index >= v->size)
        return NULL;
    return slot(v, index);
}

int vec_get(const ds_vector *v, size_t index, void *out)
{
    if (!v || !out)
        return DS_ERR_INVALID;
    if (index >= v->size)
        return DS_ERR_RANGE;
    memcpy(out, slot(v, index), v->elem_size);
    return DS_OK;
}

int vec_set(ds_vector *v, size_t index, const void *elem)
{
    if (!v || !elem)
        return DS_ERR_INVALID;
    if (index >= v->size)
        return DS_ERR_RANGE;
    memmove(slot(v, index), elem, v->elem_size); /* elem may overlap the slot */
    return DS_OK;
}

int vec_insert(ds_vector *v, size_t index, const void *elem)
{
    const unsigned char *src;
    unsigned char *at;
    int rc;

    if (!v || !elem)
        return DS_ERR_INVALID;
    if (index > v->size)
        return DS_ERR_RANGE;
    if ((rc = ensure_room(v, &elem)) != DS_OK)
        return rc;

    at = slot(v, index);
    src = elem;
    memmove(at + v->elem_size, at, (v->size - index) * v->elem_size);

    /* If elem lived in the part we just shifted, it moved one slot right. */
    if ((uintptr_t)src >= (uintptr_t)at && (uintptr_t)src < (uintptr_t)slot(v, v->size))
        src += v->elem_size;
    memcpy(at, src, v->elem_size);
    v->size++;
    return DS_OK;
}

int vec_push_back(ds_vector *v, const void *elem)
{
    return vec_insert(v, vec_size(v), elem);
}

int vec_erase(ds_vector *v, size_t index, void *out)
{
    unsigned char *at;

    if (!v)
        return DS_ERR_INVALID;
    if (index >= v->size)
        return DS_ERR_RANGE;

    at = slot(v, index);
    if (out)
        memcpy(out, at, v->elem_size);
    memmove(at, at + v->elem_size, (v->size - index - 1) * v->elem_size);
    v->size--;
    return DS_OK;
}

int vec_pop_back(ds_vector *v, void *out)
{
    if (!v)
        return DS_ERR_INVALID;
    if (v->size == 0)
        return DS_ERR_EMPTY;
    return vec_erase(v, v->size - 1, out);
}

int vec_reserve(ds_vector *v, size_t capacity)
{
    if (!v)
        return DS_ERR_INVALID;
    if (capacity <= v->capacity)
        return DS_OK;
    return set_capacity(v, capacity);
}

int vec_shrink_to_fit(ds_vector *v)
{
    if (!v)
        return DS_ERR_INVALID;
    return set_capacity(v, v->size);
}

void vec_clear(ds_vector *v)
{
    if (v)
        v->size = 0;
}

size_t vec_find(const ds_vector *v, const void *elem, ds_cmp_fn cmp)
{
    size_t i;

    if (!v || !elem || !cmp)
        return DS_VEC_NPOS;
    for (i = 0; i < v->size; i++)
        if (cmp(slot(v, i), elem) == 0)
            return i;
    return DS_VEC_NPOS;
}

void vec_sort(ds_vector *v, ds_cmp_fn cmp)
{
    if (v && cmp && v->size > 1)
        qsort(v->data, v->size, v->elem_size, cmp);
}

/* Lower-bound binary search: returns the FIRST matching index. */
size_t vec_bsearch(const ds_vector *v, const void *elem, ds_cmp_fn cmp)
{
    size_t lo = 0, hi;

    if (!v || !elem || !cmp)
        return DS_VEC_NPOS;

    hi = v->size;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2; /* (lo + hi) / 2 could overflow */
        if (cmp(slot(v, mid), elem) < 0)
            lo = mid + 1;
        else
            hi = mid;
    }
    return (lo < v->size && cmp(slot(v, lo), elem) == 0) ? lo : DS_VEC_NPOS;
}
