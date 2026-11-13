#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_aligned_alloc(size_t alignment, size_t size);
void __real_free(void *ptr);

static void *live[16];
static unsigned attempts;
static unsigned allocated;
static unsigned freed;
static unsigned anomalous;
static bool initialized;
static long fail_at;

static void report(void)
{
    unsigned remaining = 0;
    for (size_t i = 0; i < 16; ++i) {
        if (live[i] != NULL) {
            ++remaining;
        }
    }
    fprintf(stderr,
            "R1395_PHYSICAL attempts=%u allocations=%u frees=%u live=%u "
            "anomalous=%u\n",
            attempts, allocated, freed, remaining, anomalous);
}

static void setup(void)
{
    if (initialized) {
        return;
    }
    initialized = true;
    const char *setting = getenv("R1395_FAIL_AT");
    fail_at = setting == NULL ? 0 : strtol(setting, NULL, 10);
    if (atexit(report) != 0) {
        ++anomalous;
    }
}

static bool should_fail(void)
{
    setup();
    ++attempts;
    return fail_at > 0 && (long)attempts == fail_at;
}

static void record(void *ptr)
{
    if (ptr == NULL) {
        return;
    }
    ++allocated;
    for (size_t i = 0; i < 16; ++i) {
        if (live[i] == NULL) {
            live[i] = ptr;
            return;
        }
    }
    ++anomalous;
}

void *__wrap_malloc(size_t size)
{
    if (should_fail()) {
        return NULL;
    }
    void *ptr = __real_malloc(size);
    record(ptr);
    return ptr;
}

void *__wrap_calloc(size_t count, size_t size)
{
    if (should_fail()) {
        return NULL;
    }
    void *ptr = __real_calloc(count, size);
    record(ptr);
    return ptr;
}

void *__wrap_aligned_alloc(size_t alignment, size_t size)
{
    if (should_fail()) {
        return NULL;
    }
    void *ptr = __real_aligned_alloc(alignment, size);
    record(ptr);
    return ptr;
}

void __wrap_free(void *ptr)
{
    if (ptr != NULL) {
        bool found = false;
        for (size_t i = 0; i < 16; ++i) {
            if (live[i] == ptr) {
                live[i] = NULL;
                ++freed;
                found = true;
                break;
            }
        }
        if (!found) {
            ++anomalous;
        }
    }
    __real_free(ptr);
}
