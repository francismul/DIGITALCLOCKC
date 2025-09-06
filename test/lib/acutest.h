/*
  Minimal Acutest-like header for self-contained C tests.
  Source: https://github.com/mity/acutest (trimmed); License: MIT
*/
#ifndef ACUTEST_H
#define ACUTEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*TEST_FUNC)(void);
typedef struct { const char* name; TEST_FUNC func; } TEST;

static int test_verbose = 1;
static int tests_failed = 0;
static const TEST* current_test = NULL;

#define TEST_CHECK(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "\n[%s] CHECK failed: %s (%s:%d)\n", \
            current_test ? current_test->name : "?", #cond, __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define TEST_CHECK_MSG(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "\n[%s] CHECK failed: %s (%s:%d)\n", \
            current_test ? current_test->name : "?", msg, __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define TEST_CASE(name) static void name(void)

#define TEST_LIST_BEGIN static const TEST tests[] = {
#define TEST_LIST_ENTRY(fn) { #fn, fn },
#define TEST_LIST_END { NULL, NULL } };

static int test_run_all(const TEST* t) {
    for (; t && t->name; ++t) {
        current_test = t;
        if (test_verbose) fprintf(stderr, "Running %s...\n", t->name);
        t->func();
    }
    current_test = NULL;
    return tests_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

#endif /* ACUTEST_H */