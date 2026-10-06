/*
 * ds_vector - generic dynamic array.
 *
 * Elements of any type are stored by value in one contiguous buffer.
 * Capacity doubles when full, so push_back is amortized O(1).
 *
 * Pointers returned by vec_at() / vec_data() are invalidated by any call
 * that can grow the vector (push_back, insert, reserve).
 */
#ifndef DS_VECTOR_H
#define DS_VECTOR_H

#include <stddef.h>
#include "ds_common.h"

#define DS_VEC_NPOS ((size_t)-1) /* "not found" index */

typedef struct ds_vector ds_vector;

ds_vector *vec_create(size_t elem_size, size_t initial_capacity);
void       vec_destroy(ds_vector *v);

size_t vec_size(const ds_vector *v);
size_t vec_capacity(const ds_vector *v);
int    vec_empty(const ds_vector *v);
void  *vec_data(const ds_vector *v);
void  *vec_at(const ds_vector *v, size_t index);        /* NULL if out of range */

int vec_get(const ds_vector *v, size_t index, void *out); /* copy element out    */
int vec_set(ds_vector *v, size_t index, const void *elem);
int vec_push_back(ds_vector *v, const void *elem);          /* amortized O(1)     */
int vec_pop_back(ds_vector *v, void *out);                  /* out may be NULL    */
int vec_insert(ds_vector *v, size_t index, const void *elem); /* O(n), index <= size */
int vec_erase(ds_vector *v, size_t index, void *out);        /* O(n), out may be NULL */

int  vec_reserve(ds_vector *v, size_t capacity);
int  vec_shrink_to_fit(ds_vector *v);
void vec_clear(ds_vector *v);

size_t vec_find(const ds_vector *v, const void *elem, ds_cmp_fn cmp);    /* O(n)     */
void   vec_sort(ds_vector *v, ds_cmp_fn cmp);                            /* O(n log n) */
size_t vec_bsearch(const ds_vector *v, const void *elem, ds_cmp_fn cmp); /* O(log n), sorted only */

#endif /* DS_VECTOR_H */
