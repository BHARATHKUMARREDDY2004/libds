#include "ds_vector.h"
#include "test.h"

#include <string.h>

static int cmp_int(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

static void test_push_get_pop(void)
{
    ds_vector *v = vec_create(sizeof(int), 0);
    int i, x, ok = 1;

    CHECK(v != NULL && vec_empty(v));
    for (i = 0; i < 100000; i++)
        ok &= vec_push_back(v, &i) == DS_OK;
    CHECK(ok);
    CHECK(vec_size(v) == 100000);

    for (i = 0; i < 100000; i++)
        ok &= *(int *)vec_at(v, (size_t)i) == i;
    CHECK(ok);

    CHECK(vec_pop_back(v, &x) == DS_OK && x == 99999);
    CHECK(vec_get(v, 0, &x) == DS_OK && x == 0);
    CHECK(vec_get(v, vec_size(v), &x) == DS_ERR_RANGE);
    CHECK(vec_at(v, vec_size(v)) == NULL);
    vec_destroy(v);
}

static void test_growth_is_geometric(void)
{
    ds_vector *v = vec_create(sizeof(int), 0);
    size_t reallocs = 0, last_cap = 0;
    int i;

    for (i = 0; i < 1000000; i++) {
        vec_push_back(v, &i);
        if (vec_capacity(v) != last_cap) {
            reallocs++;
            last_cap = vec_capacity(v);
        }
    }
    CHECK(reallocs <= 20);              /* ~log2(1e6 / 8), not 1e6 */
    CHECK(vec_capacity(v) >= vec_size(v));
    CHECK(vec_capacity(v) < 2 * vec_size(v) + 8);

    CHECK(vec_shrink_to_fit(v) == DS_OK);
    CHECK(vec_capacity(v) == vec_size(v));
    vec_destroy(v);
}

static void test_insert_erase(void)
{
    ds_vector *v = vec_create(sizeof(int), 2);
    int a = 1, b = 2, c = 3, d = 4, x;

    CHECK(vec_insert(v, 0, &b) == DS_OK);          /* [2]       */
    CHECK(vec_insert(v, 0, &a) == DS_OK);          /* [1 2]     */
    CHECK(vec_insert(v, 2, &d) == DS_OK);          /* [1 2 4]   */
    CHECK(vec_insert(v, 2, &c) == DS_OK);          /* [1 2 3 4] */
    CHECK(vec_insert(v, 9, &c) == DS_ERR_RANGE);
    CHECK(vec_size(v) == 4);
    CHECK(memcmp(vec_data(v), (int[]){1, 2, 3, 4}, 4 * sizeof(int)) == 0);

    CHECK(vec_erase(v, 1, &x) == DS_OK && x == 2); /* [1 3 4] */
    CHECK(vec_erase(v, 2, NULL) == DS_OK);         /* [1 3]   */
    CHECK(vec_erase(v, 2, NULL) == DS_ERR_RANGE);
    CHECK(memcmp(vec_data(v), (int[]){1, 3}, 2 * sizeof(int)) == 0);

    CHECK(vec_set(v, 1, &d) == DS_OK && *(int *)vec_at(v, 1) == 4);
    vec_destroy(v);
}

static void test_empty_and_invalid(void)
{
    ds_vector *v = vec_create(sizeof(int), 0);
    int x = 0;

    CHECK(vec_create(0, 10) == NULL);
    CHECK(vec_pop_back(v, &x) == DS_ERR_EMPTY);
    CHECK(vec_push_back(NULL, &x) == DS_ERR_INVALID);
    CHECK(vec_push_back(v, NULL) == DS_ERR_INVALID);
    CHECK(vec_size(NULL) == 0);
    vec_destroy(NULL); /* must not crash */
    vec_destroy(v);
}

static void test_self_aliasing(void)
{
    ds_vector *v = vec_create(sizeof(int), 0);
    int i;

    for (i = 0; i < 8; i++) /* exactly full: the next push reallocates */
        vec_push_back(v, &i);
    CHECK(vec_size(v) == vec_capacity(v));

    CHECK(vec_push_back(v, vec_at(v, 3)) == DS_OK); /* source moves during realloc */
    CHECK(*(int *)vec_at(v, 8) == 3);

    CHECK(vec_insert(v, 0, vec_at(v, 5)) == DS_OK); /* source shifts during memmove */
    CHECK(*(int *)vec_at(v, 0) == 5);
    CHECK(*(int *)vec_at(v, 6) == 5);
    vec_destroy(v);
}

typedef struct {
    double x, y;
    char   tag[12];
} point;

static void test_struct_elements(void)
{
    ds_vector *v = vec_create(sizeof(point), 0);
    point p, out;
    int i, ok = 1;

    for (i = 0; i < 50; i++) {
        p.x = i;
        p.y = -i;
        sprintf(p.tag, "pt-%d", i);
        vec_push_back(v, &p);
    }
    for (i = 0; i < 50; i++) {
        point *q = vec_at(v, (size_t)i);
        char tag[12];
        sprintf(tag, "pt-%d", i);
        ok &= q->x == i && q->y == -i && strcmp(q->tag, tag) == 0;
    }
    CHECK(ok);
    CHECK(vec_pop_back(v, &out) == DS_OK && out.x == 49 && strcmp(out.tag, "pt-49") == 0);
    vec_destroy(v);
}

static void test_find_sort_bsearch(void)
{
    ds_vector *v = vec_create(sizeof(int), 0);
    int data[] = {42, 7, 19, 7, 3, 100, -5, 19};
    int key, i, sorted = 1;

    for (i = 0; i < 8; i++)
        vec_push_back(v, &data[i]);

    key = 19;
    CHECK(vec_find(v, &key, cmp_int) == 2);
    key = 1000;
    CHECK(vec_find(v, &key, cmp_int) == DS_VEC_NPOS);

    vec_sort(v, cmp_int); /* [-5 3 7 7 19 19 42 100] */
    for (i = 1; i < 8; i++)
        sorted &= *(int *)vec_at(v, (size_t)i - 1) <= *(int *)vec_at(v, (size_t)i);
    CHECK(sorted);

    key = 7;
    CHECK(vec_bsearch(v, &key, cmp_int) == 2); /* first of the duplicates */
    key = 100;
    CHECK(vec_bsearch(v, &key, cmp_int) == 7);
    key = -5;
    CHECK(vec_bsearch(v, &key, cmp_int) == 0);
    key = 8;
    CHECK(vec_bsearch(v, &key, cmp_int) == DS_VEC_NPOS);
    vec_destroy(v);
}

int main(void)
{
    printf("ds_vector tests\n");
    RUN(test_push_get_pop);
    RUN(test_growth_is_geometric);
    RUN(test_insert_erase);
    RUN(test_empty_and_invalid);
    RUN(test_self_aliasing);
    RUN(test_struct_elements);
    RUN(test_find_sort_bsearch);
    return TEST_REPORT();
}
