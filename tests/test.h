/* Minimal unit-test helpers shared by the test programs. */
#ifndef TEST_H
#define TEST_H

#include <stdio.h>

static int g_checks, g_failures;

#define CHECK(cond)                                                          \
    do {                                                                     \
        g_checks++;                                                          \
        if (!(cond)) {                                                       \
            g_failures++;                                                    \
            fprintf(stderr, "    FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                    \
    } while (0)

#define RUN(test)                                                            \
    do {                                                                     \
        int before_ = g_failures;                                            \
        test();                                                              \
        printf("  %-36s %s\n", #test, g_failures == before_ ? "PASS" : "FAIL"); \
    } while (0)

#define TEST_REPORT()                                                        \
    (printf("%d checks, %d failures\n", g_checks, g_failures), g_failures ? 1 : 0)

#endif /* TEST_H */
