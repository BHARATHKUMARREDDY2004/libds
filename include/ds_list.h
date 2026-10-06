/*
 * ds_list - generic doubly linked list.
 *
 * Each node stores a copy of the element inline (one allocation per node).
 * O(1) push/pop at both ends and O(1) removal of a known node.
 *
 * Iterate with:
 *   for (ds_list_node *n = list_head(l); n; n = list_next(n))
 *       use(list_data(n));
 */
#ifndef DS_LIST_H
#define DS_LIST_H

#include <stddef.h>
#include "ds_common.h"

typedef struct ds_list ds_list;
typedef struct ds_list_node ds_list_node;

ds_list *list_create(size_t elem_size);
void     list_destroy(ds_list *l);

size_t list_size(const ds_list *l);
int    list_empty(const ds_list *l);

int list_push_front(ds_list *l, const void *elem);
int list_push_back(ds_list *l, const void *elem);
int list_pop_front(ds_list *l, void *out);   /* out may be NULL */
int list_pop_back(ds_list *l, void *out);

void *list_front(const ds_list *l);          /* NULL if empty */
void *list_back(const ds_list *l);
void *list_at(const ds_list *l, size_t index); /* O(n/2): walks from the nearer end */

int list_insert_at(ds_list *l, size_t index, const void *elem); /* index <= size */
int list_remove_at(ds_list *l, size_t index, void *out);

ds_list_node *list_find(const ds_list *l, const void *elem, ds_cmp_fn cmp);
int           list_remove_node(ds_list *l, ds_list_node *node, void *out); /* O(1) */

void list_reverse(ds_list *l);
void list_clear(ds_list *l);

/* Iteration */
ds_list_node *list_head(const ds_list *l);
ds_list_node *list_tail(const ds_list *l);
ds_list_node *list_next(const ds_list_node *node);
ds_list_node *list_prev(const ds_list_node *node);
void         *list_data(ds_list_node *node);

#endif /* DS_LIST_H */
