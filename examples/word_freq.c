/*
 * word_freq - counts words in a text file using all three libds containers.
 *
 *   ds_hashmap  word -> count            (O(1) average lookup per word)
 *   ds_list     sliding window of the last few words (push back, pop front)
 *   ds_vector   (word, count) pairs, sorted by frequency for the report
 *
 * Usage: word_freq [file]      (reads stdin when no file is given)
 */
#include "ds_hashmap.h"
#include "ds_list.h"
#include "ds_vector.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define MAX_WORD 32
#define TOP_N    10
#define RECENT_N 5

typedef struct {
    const char *word; /* points at the key stored inside the hash map */
    int         count;
} word_count;

static int by_count_desc(const void *a, const void *b)
{
    const word_count *x = a, *y = b;

    if (x->count != y->count)
        return y->count > x->count ? 1 : -1;
    return strcmp(x->word, y->word);
}

/* Read the next word (letters only, lower-cased). Returns 0 at end of input. */
static int next_word(FILE *in, char word[MAX_WORD])
{
    size_t n = 0;
    int c;

    while ((c = fgetc(in)) != EOF && !isalpha(c))
        ;
    if (c == EOF)
        return 0;
    do {
        if (n + 1 < MAX_WORD)
            word[n++] = (char)tolower(c);
    } while ((c = fgetc(in)) != EOF && isalpha(c));
    word[n] = '\0';
    return 1;
}

int main(int argc, char **argv)
{
    FILE *in = stdin;
    ds_hashmap *counts = hm_create(sizeof(int), 256);
    ds_list *recent = list_create(MAX_WORD);
    ds_vector *ranking = vec_create(sizeof(word_count), 0);
    ds_hashmap_iter it;
    ds_list_node *node;
    char word[MAX_WORD];
    const char *key;
    void *val;
    long total = 0;
    size_t i;
    int status = 0;

    if (!counts || !recent || !ranking) {
        fputs("out of memory\n", stderr);
        return 1;
    }
    if (argc > 1 && !(in = fopen(argv[1], "r"))) {
        perror(argv[1]);
        return 1;
    }

    while (next_word(in, word)) {
        int *count = hm_get(counts, word);
        int one = 1;

        total++;
        if (count)
            (*count)++;
        else if (hm_put(counts, word, &one) != DS_OK)
            status = 1;

        if (list_push_back(recent, word) != DS_OK)
            status = 1;
        if (list_size(recent) > RECENT_N)
            list_pop_front(recent, NULL);
    }
    if (in != stdin)
        fclose(in);

    hm_iter_init(counts, &it);
    while (hm_iter_next(&it, &key, &val)) {
        word_count wc;
        wc.word = key;
        wc.count = *(int *)val;
        if (vec_push_back(ranking, &wc) != DS_OK)
            status = 1;
    }
    vec_sort(ranking, by_count_desc);

    printf("%ld words, %lu unique\n\n", total, (unsigned long)hm_size(counts));
    printf("Top %d:\n", TOP_N);
    for (i = 0; i < TOP_N && i < vec_size(ranking); i++) {
        word_count *wc = vec_at(ranking, i);
        printf("  %2lu. %-15s %d\n", (unsigned long)i + 1, wc->word, wc->count);
    }

    printf("\nLast %d words:", RECENT_N);
    for (node = list_head(recent); node; node = list_next(node))
        printf(" %s", (char *)list_data(node));

    printf("\n\nHash map: %lu buckets, load factor %.2f, longest chain %lu\n",
           (unsigned long)hm_bucket_count(counts), hm_load_factor(counts),
           (unsigned long)hm_longest_chain(counts));

    vec_destroy(ranking);
    list_destroy(recent);
    hm_destroy(counts);
    if (status)
        fputs("warning: some words were dropped (out of memory)\n", stderr);
    return status;
}
