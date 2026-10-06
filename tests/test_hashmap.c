#include "ds_hashmap.h"
#include "test.h"

#include <string.h>

static void test_fnv1a_known_values(void)
{
    CHECK(ds_hash_str("") == 0x811c9dc5u);
    CHECK(ds_hash_str("a") == 0xe40c292cu);
    CHECK(ds_hash_str("foobar") == 0xbf9cf968u);
}

static void test_put_get_overwrite(void)
{
    ds_hashmap *m = hm_create(sizeof(int), 0);
    int one = 1, two = 2, x;

    CHECK(hm_put(m, "apple", &one) == DS_OK);
    CHECK(hm_size(m) == 1);
    CHECK(*(int *)hm_get(m, "apple") == 1);
    CHECK(hm_get(m, "banana") == NULL);
    CHECK(!hm_contains(m, "banana"));

    CHECK(hm_put(m, "apple", &two) == DS_OK); /* overwrite, no new entry */
    CHECK(hm_size(m) == 1);
    CHECK(*(int *)hm_get(m, "apple") == 2);

    CHECK(hm_put(m, "", &one) == DS_OK); /* empty string is a valid key */
    CHECK(*(int *)hm_get(m, "") == 1);

    CHECK(hm_remove(m, "apple", &x) == DS_OK && x == 2);
    CHECK(hm_remove(m, "apple", NULL) == DS_ERR_NOTFOUND);
    CHECK(hm_size(m) == 1);

    CHECK(hm_put(NULL, "k", &one) == DS_ERR_INVALID);
    CHECK(hm_put(m, NULL, &one) == DS_ERR_INVALID);
    CHECK(hm_put(m, "k", NULL) == DS_ERR_INVALID);
    hm_destroy(m);
}

static void test_keys_are_copied(void)
{
    ds_hashmap *m = hm_create(sizeof(int), 0);
    char buf[16] = "alpha";
    int v = 5;

    hm_put(m, buf, &v);
    strcpy(buf, "beta"); /* caller reuses its buffer */
    CHECK(hm_contains(m, "alpha"));
    CHECK(!hm_contains(m, "beta"));
    hm_destroy(m);
}

static void test_resize_keeps_everything(void)
{
    ds_hashmap *m = hm_create(sizeof(int), 0);
    char key[32];
    int i, ok = 1;

    CHECK(hm_bucket_count(m) == 8);
    for (i = 0; i < 20000; i++) {
        sprintf(key, "key-%d", i);
        ok &= hm_put(m, key, &i) == DS_OK;
        ok &= hm_load_factor(m) <= 0.75;
    }
    CHECK(ok);
    CHECK(hm_size(m) == 20000);
    CHECK(hm_bucket_count(m) == 32768); /* power of two, 20000/32768 = 0.61 */

    for (i = 0; i < 20000; i++) {
        int *v;
        sprintf(key, "key-%d", i);
        v = hm_get(m, key);
        ok &= v && *v == i;
    }
    CHECK(ok);
    CHECK(hm_longest_chain(m) < 10); /* FNV-1a spreads the keys well */
    hm_destroy(m);
}

static void test_presized_map_does_not_resize(void)
{
    ds_hashmap *m = hm_create(sizeof(int), 1000);
    size_t buckets = hm_bucket_count(m);
    char key[16];
    int i;

    CHECK(buckets == 2048); /* 1000 / 0.75 = 1334 -> next power of two */
    for (i = 0; i < 1000; i++) {
        sprintf(key, "%d", i);
        hm_put(m, key, &i);
    }
    CHECK(hm_bucket_count(m) == buckets);
    hm_destroy(m);
}

/* Find keys that land in the same bucket of an 8-bucket table. */
static void test_collision_chain(void)
{
    ds_hashmap *m = hm_create(sizeof(int), 0);
    char keys[3][16];
    char key[16];
    int found = 0, i, v;

    for (i = 0; found < 3; i++) {
        sprintf(key, "k%d", i);
        if ((ds_hash_str(key) & 7) == 0)
            strcpy(keys[found++], key);
    }
    for (i = 0; i < 3; i++)
        hm_put(m, keys[i], &i);
    CHECK(hm_bucket_count(m) == 8);
    CHECK(hm_longest_chain(m) == 3);

    CHECK(hm_remove(m, keys[1], &v) == DS_OK && v == 1); /* middle of the chain */
    CHECK(*(int *)hm_get(m, keys[0]) == 0);
    CHECK(*(int *)hm_get(m, keys[2]) == 2);
    CHECK(hm_remove(m, keys[0], NULL) == DS_OK);          /* head of the chain */
    CHECK(*(int *)hm_get(m, keys[2]) == 2);
    CHECK(hm_longest_chain(m) == 1);
    hm_destroy(m);
}

static void test_iteration(void)
{
    ds_hashmap *m = hm_create(sizeof(int), 0);
    ds_hashmap_iter it;
    const char *key;
    void *val;
    char buf[16];
    long sum = 0;
    int i, count = 0, pairs_match = 1;

    for (i = 0; i < 500; i++) {
        sprintf(buf, "n%d", i);
        hm_put(m, buf, &i);
    }

    hm_iter_init(m, &it);
    while (hm_iter_next(&it, &key, &val)) {
        sprintf(buf, "n%d", *(int *)val);
        pairs_match &= strcmp(key, buf) == 0; /* key and value belong together */
        sum += *(int *)val;
        count++;
    }
    CHECK(pairs_match);
    CHECK(count == 500);
    CHECK(sum == 499L * 500 / 2); /* every entry visited exactly once */

    /* Removing the entry just returned is allowed. */
    hm_iter_init(m, &it);
    while (hm_iter_next(&it, &key, &val))
        if (*(int *)val % 2 == 0)
            hm_remove(m, key, NULL);
    CHECK(hm_size(m) == 250);
    CHECK(!hm_contains(m, "n0") && hm_contains(m, "n1"));

    hm_clear(m);
    hm_iter_init(m, &it);
    CHECK(hm_size(m) == 0 && !hm_iter_next(&it, &key, &val));
    hm_destroy(m);
}

typedef struct {
    double     score;
    long long  id;
    char       grade;
} record;

static void test_struct_values(void)
{
    ds_hashmap *m = hm_create(sizeof(record), 0);
    record r = {91.5, 1234567890123LL, 'A'}, *got;

    hm_put(m, "alice", &r);
    got = hm_get(m, "alice");
    CHECK(got && got->score == 91.5 && got->id == 1234567890123LL && got->grade == 'A');
    hm_destroy(m);
}

static void test_as_set(void)
{
    ds_hashmap *set = hm_create(0, 0); /* val_size 0: keys only */

    CHECK(hm_put(set, "x", NULL) == DS_OK);
    CHECK(hm_put(set, "x", NULL) == DS_OK);
    CHECK(hm_size(set) == 1);
    CHECK(hm_contains(set, "x") && !hm_contains(set, "y"));
    hm_destroy(set);
}

int main(void)
{
    printf("ds_hashmap tests\n");
    RUN(test_fnv1a_known_values);
    RUN(test_put_get_overwrite);
    RUN(test_keys_are_copied);
    RUN(test_resize_keeps_everything);
    RUN(test_presized_map_does_not_resize);
    RUN(test_collision_chain);
    RUN(test_iteration);
    RUN(test_struct_values);
    RUN(test_as_set);
    return TEST_REPORT();
}
