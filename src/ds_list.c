#include "ds_list.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct ds_list_node {
    struct ds_list_node *prev;
    struct ds_list_node *next;
    max_align_t data[]; /* element bytes, aligned for any type (C11 flexible array) */
};

struct ds_list {
    ds_list_node *head;
    ds_list_node *tail;
    size_t        size;
    size_t        elem_size;
};

static ds_list_node *node_new(const ds_list *l, const void *elem)
{
    ds_list_node *n = malloc(sizeof *n + l->elem_size); /* node + element in one block */

    if (!n)
        return NULL;
    n->prev = n->next = NULL;
    memcpy(n->data, elem, l->elem_size);
    return n;
}

/* Link `n` in front of `before`; before == NULL appends at the tail. */
static void link_before(ds_list *l, ds_list_node *n, ds_list_node *before)
{
    n->next = before;
    n->prev = before ? before->prev : l->tail;

    if (n->prev)
        n->prev->next = n;
    else
        l->head = n;

    if (before)
        before->prev = n;
    else
        l->tail = n;

    l->size++;
}

static void unlink_node(ds_list *l, ds_list_node *n)
{
    if (n->prev)
        n->prev->next = n->next;
    else
        l->head = n->next;

    if (n->next)
        n->next->prev = n->prev;
    else
        l->tail = n->prev;

    l->size--;
}

/* Walk from whichever end is closer: at most size/2 steps. Requires index < size. */
static ds_list_node *node_at(const ds_list *l, size_t index)
{
    ds_list_node *n;
    size_t i;

    if (index < l->size / 2) {
        for (n = l->head, i = 0; i < index; i++)
            n = n->next;
    } else {
        for (n = l->tail, i = l->size - 1; i > index; i--)
            n = n->prev;
    }
    return n;
}

ds_list *list_create(size_t elem_size)
{
    ds_list *l;

    if (elem_size == 0 || elem_size > SIZE_MAX - sizeof(ds_list_node))
        return NULL;
    l = calloc(1, sizeof *l);
    if (l)
        l->elem_size = elem_size;
    return l;
}

void list_destroy(ds_list *l)
{
    list_clear(l);
    free(l);
}

void list_clear(ds_list *l)
{
    ds_list_node *n, *next;

    if (!l)
        return;
    for (n = l->head; n; n = next) {
        next = n->next;
        free(n);
    }
    l->head = l->tail = NULL;
    l->size = 0;
}

size_t list_size(const ds_list *l) { return l ? l->size : 0; }
int list_empty(const ds_list *l) { return list_size(l) == 0; }

int list_insert_at(ds_list *l, size_t index, const void *elem)
{
    ds_list_node *n;

    if (!l || !elem)
        return DS_ERR_INVALID;
    if (index > l->size)
        return DS_ERR_RANGE;
    n = node_new(l, elem);
    if (!n)
        return DS_ERR_NOMEM;
    link_before(l, n, index == l->size ? NULL : node_at(l, index));
    return DS_OK;
}

int list_push_front(ds_list *l, const void *elem) { return list_insert_at(l, 0, elem); }
int list_push_back(ds_list *l, const void *elem) { return list_insert_at(l, list_size(l), elem); }

int list_remove_node(ds_list *l, ds_list_node *node, void *out)
{
    if (!l || !node)
        return DS_ERR_INVALID;
    unlink_node(l, node);
    if (out)
        memcpy(out, node->data, l->elem_size);
    free(node);
    return DS_OK;
}

int list_remove_at(ds_list *l, size_t index, void *out)
{
    if (!l)
        return DS_ERR_INVALID;
    if (index >= l->size)
        return DS_ERR_RANGE;
    return list_remove_node(l, node_at(l, index), out);
}

int list_pop_front(ds_list *l, void *out)
{
    if (!l)
        return DS_ERR_INVALID;
    if (!l->head)
        return DS_ERR_EMPTY;
    return list_remove_node(l, l->head, out);
}

int list_pop_back(ds_list *l, void *out)
{
    if (!l)
        return DS_ERR_INVALID;
    if (!l->tail)
        return DS_ERR_EMPTY;
    return list_remove_node(l, l->tail, out);
}

void *list_front(const ds_list *l) { return l && l->head ? l->head->data : NULL; }
void *list_back(const ds_list *l) { return l && l->tail ? l->tail->data : NULL; }

void *list_at(const ds_list *l, size_t index)
{
    if (!l || index >= l->size)
        return NULL;
    return node_at(l, index)->data;
}

ds_list_node *list_find(const ds_list *l, const void *elem, ds_cmp_fn cmp)
{
    ds_list_node *n;

    if (!l || !elem || !cmp)
        return NULL;
    for (n = l->head; n; n = n->next)
        if (cmp(n->data, elem) == 0)
            return n;
    return NULL;
}

/* Swap prev/next in every node, then swap head and tail. O(n), no allocation. */
void list_reverse(ds_list *l)
{
    ds_list_node *n, *tmp;

    if (!l)
        return;
    for (n = l->head; n; n = n->prev) { /* n->prev is the old next after the swap */
        tmp = n->next;
        n->next = n->prev;
        n->prev = tmp;
    }
    tmp = l->head;
    l->head = l->tail;
    l->tail = tmp;
}

ds_list_node *list_head(const ds_list *l) { return l ? l->head : NULL; }
ds_list_node *list_tail(const ds_list *l) { return l ? l->tail : NULL; }
ds_list_node *list_next(const ds_list_node *node) { return node ? node->next : NULL; }
ds_list_node *list_prev(const ds_list_node *node) { return node ? node->prev : NULL; }
void *list_data(ds_list_node *node) { return node ? node->data : NULL; }
