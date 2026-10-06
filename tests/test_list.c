#include "ds_list.h"
#include "test.h"

#include <stdlib.h>
#include <string.h>

static int cmp_int(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

/* Compare the list with an array, walking it both forwards and backwards. */
static int list_equals(const ds_list *l, const int *expected, size_t n)
{
    ds_list_node *node;
    size_t i;

    if (list_size(l) != n)
        return 0;
    for (i = 0, node = list_head(l); node; node = list_next(node), i++)
        if (i >= n || *(int *)list_data(node) != expected[i])
            return 0;
    if (i != n)
        return 0;
    for (node = list_tail(l); node; node = list_prev(node))
        if (i == 0 || *(int *)list_data(node) != expected[--i])
            return 0;
    return i == 0;
}

static void test_push_pop_both_ends(void)
{
    ds_list *l = list_create(sizeof(int));
    int a = 1, b = 2, c = 3, x;

    CHECK(list_empty(l));
    CHECK(list_push_back(l, &b) == DS_OK);  /* [2]     */
    CHECK(list_push_front(l, &a) == DS_OK); /* [1 2]   */
    CHECK(list_push_back(l, &c) == DS_OK);  /* [1 2 3] */
    CHECK(list_equals(l, (int[]){1, 2, 3}, 3));
    CHECK(*(int *)list_front(l) == 1 && *(int *)list_back(l) == 3);

    CHECK(list_pop_front(l, &x) == DS_OK && x == 1);
    CHECK(list_pop_back(l, &x) == DS_OK && x == 3);
    CHECK(list_pop_back(l, &x) == DS_OK && x == 2);
    CHECK(list_empty(l) && list_head(l) == NULL && list_tail(l) == NULL);

    CHECK(list_pop_front(l, &x) == DS_ERR_EMPTY);
    CHECK(list_pop_back(l, NULL) == DS_ERR_EMPTY);
    CHECK(list_front(l) == NULL && list_back(l) == NULL);
    list_destroy(l);
}

static void test_index_operations(void)
{
    ds_list *l = list_create(sizeof(int));
    int i, x, ok = 1;

    for (i = 0; i < 10; i++)
        list_push_back(l, &i);
    for (i = 0; i < 10; i++) /* both halves: walks from head and from tail */
        ok &= *(int *)list_at(l, (size_t)i) == i;
    CHECK(ok);
    CHECK(list_at(l, 10) == NULL);

    x = 100;
    CHECK(list_insert_at(l, 0, &x) == DS_OK);
    CHECK(list_insert_at(l, 11, &x) == DS_OK); /* == size: append */
    CHECK(list_insert_at(l, 6, &x) == DS_OK);
    CHECK(list_insert_at(l, 99, &x) == DS_ERR_RANGE);
    CHECK(list_equals(l, (int[]){100, 0, 1, 2, 3, 4, 100, 5, 6, 7, 8, 9, 100}, 13));

    CHECK(list_remove_at(l, 6, &x) == DS_OK && x == 100);
    CHECK(list_remove_at(l, 0, NULL) == DS_OK);
    CHECK(list_remove_at(l, 10, NULL) == DS_OK);
    CHECK(list_remove_at(l, 10, NULL) == DS_ERR_RANGE);
    CHECK(list_equals(l, (int[]){0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, 10));
    list_destroy(l);
}

static void test_reverse(void)
{
    ds_list *l = list_create(sizeof(int));
    int i;

    list_reverse(l); /* empty */
    CHECK(list_empty(l));

    i = 7;
    list_push_back(l, &i);
    list_reverse(l); /* single node */
    CHECK(list_equals(l, (int[]){7}, 1));

    list_clear(l);
    for (i = 1; i <= 5; i++)
        list_push_back(l, &i);
    list_reverse(l);
    CHECK(list_equals(l, (int[]){5, 4, 3, 2, 1}, 5));
    list_reverse(l);
    CHECK(list_equals(l, (int[]){1, 2, 3, 4, 5}, 5));
    list_destroy(l);
}

static void test_find_and_remove_node(void)
{
    ds_list *l = list_create(sizeof(int));
    ds_list_node *n;
    int i, key, x;

    for (i = 1; i <= 5; i++)
        list_push_back(l, &i);

    key = 3;
    n = list_find(l, &key, cmp_int);
    CHECK(n != NULL && *(int *)list_data(n) == 3);
    CHECK(list_remove_node(l, n, &x) == DS_OK && x == 3); /* middle */

    key = 1;
    CHECK(list_remove_node(l, list_find(l, &key, cmp_int), NULL) == DS_OK); /* head */
    key = 5;
    CHECK(list_remove_node(l, list_find(l, &key, cmp_int), NULL) == DS_OK); /* tail */
    CHECK(list_equals(l, (int[]){2, 4}, 2));

    key = 42;
    CHECK(list_find(l, &key, cmp_int) == NULL);
    CHECK(list_remove_node(l, NULL, NULL) == DS_ERR_INVALID);
    list_destroy(l);
}

static void test_string_elements(void)
{
    ds_list *l = list_create(16); /* fixed char[16] elements */
    char word[16];

    strcpy(word, "alpha");
    list_push_back(l, word);
    strcpy(word, "beta"); /* list holds its own copy */
    list_push_back(l, word);

    CHECK(strcmp(list_front(l), "alpha") == 0);
    CHECK(strcmp(list_back(l), "beta") == 0);
    list_destroy(l);
}

/* Random operations mirrored on a plain array; both must always agree. */
static void test_random_against_array(void)
{
    enum { CAP = 2000, OPS = 50000 };
    static int ref[CAP];
    size_t n = 0, idx;
    ds_list *l = list_create(sizeof(int));
    int i, x, ok = 1;

    srand(7);
    for (i = 0; i < OPS; i++) {
        int op = rand() % 6;
        x = rand();
        if (n == CAP)
            op = 2 + op % 2; /* full: only pop */

        switch (op) {
        case 0:
            ok &= list_push_back(l, &x) == DS_OK;
            ref[n++] = x;
            break;
        case 1:
            ok &= list_push_front(l, &x) == DS_OK;
            memmove(ref + 1, ref, n * sizeof *ref);
            ref[0] = x;
            n++;
            break;
        case 2:
            if (n) {
                ok &= list_pop_front(l, &x) == DS_OK && x == ref[0];
                memmove(ref, ref + 1, --n * sizeof *ref);
            }
            break;
        case 3:
            if (n)
                ok &= list_pop_back(l, &x) == DS_OK && x == ref[--n];
            break;
        case 4:
            idx = (size_t)rand() % (n + 1);
            ok &= list_insert_at(l, idx, &x) == DS_OK;
            memmove(ref + idx + 1, ref + idx, (n - idx) * sizeof *ref);
            ref[idx] = x;
            n++;
            break;
        default:
            if (n) {
                idx = (size_t)rand() % n;
                ok &= list_remove_at(l, idx, &x) == DS_OK && x == ref[idx];
                memmove(ref + idx, ref + idx + 1, (n - idx - 1) * sizeof *ref);
                n--;
            }
            break;
        }
        if (i % 1000 == 0)
            ok &= list_equals(l, ref, n);
    }
    CHECK(ok);
    CHECK(list_equals(l, ref, n));
    list_destroy(l);
}

int main(void)
{
    printf("ds_list tests\n");
    RUN(test_push_pop_both_ends);
    RUN(test_index_operations);
    RUN(test_reverse);
    RUN(test_find_and_remove_node);
    RUN(test_string_elements);
    RUN(test_random_against_array);
    return TEST_REPORT();
}
