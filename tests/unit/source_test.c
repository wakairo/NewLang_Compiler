#include "../support/test.h"
#include "newlang/source.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Linux test-only link wrapping: exercise real allocation/cleanup paths without
 * adding an allocator abstraction to production. Disabled outside one call. */
static size_t fail_allocation;
static size_t allocation_count;
void *__real_malloc(size_t size);
void *__wrap_malloc(size_t size)
{
    if (fail_allocation != 0 && ++allocation_count == fail_allocation) {
        return NULL;
    }
    return __real_malloc(size);
}

static bool storage_and_spans(void)
{
    static const unsigned char unusual[] = {'a',  0,    'b',  '\n', '\r',
                                            '\n', 0xef, 0xbb, 0xbf, 0xc3,
                                            0xa9, 0xff, '\r'};
    const struct {
        const void *bytes;
        size_t length;
    } cases[] = {{NULL, 0}, {"ordinary_ASCII", 14}, {unusual, sizeof(unusual)}};
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        NLSource *source = NULL;
        CHECK(nl_source_create(cases[i].bytes, cases[i].length, "memory",
                               &source) == NL_SOURCE_OK);
        CHECK(nl_source_length(source) == cases[i].length);
        CHECK(strcmp(nl_source_name(source), "memory") == 0);
        for (size_t start = 0; start <= cases[i].length; ++start) {
            for (size_t end = start; end <= cases[i].length; ++end) {
                const NLSourceSpan span = {start, end};
                NLSourceView view;
                CHECK(nl_source_span_valid(source, span));
                CHECK(nl_source_view(source, span, &view));
                CHECK(view.length == end - start);
                if (view.length != 0) {
                    CHECK(memcmp(view.bytes,
                                 (const unsigned char *)cases[i].bytes + start,
                                 view.length) == 0);
                }
            }
        }
        const NLSourceSpan invalid[] = {
            {1, 0}, {0, cases[i].length + 1}, {SIZE_MAX, SIZE_MAX}};
        for (size_t j = 0; j < sizeof(invalid) / sizeof(invalid[0]); ++j) {
            NLSourceView untouched = {unusual, sizeof(unusual)};
            CHECK(!nl_source_span_valid(source, invalid[j]));
            CHECK(!nl_source_view(source, invalid[j], &untouched));
            CHECK(untouched.bytes == unusual &&
                  untouched.length == sizeof(unusual));
        }
        CHECK(!nl_source_view(source, (NLSourceSpan){0, 0}, NULL));
        nl_source_destroy(source);
    }
    nl_source_destroy(NULL);
    CHECK(!nl_source_span_valid(NULL, (NLSourceSpan){0, 0}));
    return true;
}

static bool ownership_and_lifecycle(void)
{
    unsigned char input[] = {'x', 0, 'y'};
    char name[] = "same-name";
    NLSource *first = NULL;
    NLSource *second = NULL;
    CHECK(nl_source_create(input, sizeof(input), name, &first) == NL_SOURCE_OK);
    CHECK(nl_source_create(input, sizeof(input), name, &second) ==
          NL_SOURCE_OK);
    CHECK(first != second); /* Equal names do not merge source identities. */
    memset(input, 'z', sizeof(input));
    name[0] = 'Z';
    NLSourceView view;
    CHECK(nl_source_view(first, (NLSourceSpan){0, 3}, &view));
    CHECK(view.bytes[0] == 'x' && view.bytes[1] == 0 && view.bytes[2] == 'y');
    CHECK(strcmp(nl_source_name(first), "same-name") == 0);
    CHECK(nl_source_create("other", 5, "other", &first) ==
          NL_SOURCE_INVALID_ARGUMENT);
    CHECK(nl_source_length(first) == 3 && view.bytes[2] == 'y');
    nl_source_destroy(second);
    CHECK(view.bytes[0] ==
          'x'); /* Other object's destruction does not invalidate. */
    nl_source_destroy(first);
    for (size_t i = 0; i < 64; ++i) {
        NLSource *source = NULL;
        CHECK(nl_source_create(NULL, 0, "", &source) == NL_SOURCE_OK);
        nl_source_destroy(source);
    }
    return true;
}

static bool failures(void)
{
    NLSource *source = NULL;
    CHECK(nl_source_create(NULL, 1, "invalid", &source) ==
          NL_SOURCE_INVALID_ARGUMENT);
    CHECK(nl_source_create("a", 1, NULL, &source) ==
          NL_SOURCE_INVALID_ARGUMENT);
    CHECK(nl_source_create("a", 1, "invalid", NULL) ==
          NL_SOURCE_INVALID_ARGUMENT);
    CHECK(nl_source_create("a", SIZE_MAX, "overflow", &source) ==
          NL_SOURCE_TOO_LARGE);
    CHECK(source == NULL);
    CHECK(nl_source_load(NULL, &source) == NL_SOURCE_INVALID_ARGUMENT);
    CHECK(nl_source_load("not opened", NULL) == NL_SOURCE_INVALID_ARGUMENT);
    for (size_t length = 0; length <= 1; ++length) {
        bool saw_failure = false;
        bool succeeded = false;
        /* Stop when the injected allocation is no longer reached; do not assert
         * an implementation-specific allocation count. */
        for (size_t nth = 1; nth <= 16; ++nth) {
            fail_allocation = nth;
            allocation_count = 0;
            const NLSourceStatus status =
                nl_source_create("a", length, "oom", &source);
            fail_allocation = 0;
            if (status == NL_SOURCE_OK) {
                nl_source_destroy(source);
                source = NULL;
                succeeded = true;
                break;
            }
            CHECK(status == NL_SOURCE_OUT_OF_MEMORY && source == NULL);
            saw_failure = true;
        }
        CHECK(saw_failure && succeeded);
    }
    CHECK(nl_source_create("retry", 5, "retry", &source) == NL_SOURCE_OK);
    nl_source_destroy(source);
    return true;
}

int main(void)
{
    if (!storage_and_spans() || !ownership_and_lifecycle() || !failures()) {
        return EXIT_FAILURE;
    }
    puts("source: storage, spans, ownership, lifecycle and allocation failures "
         "passed");
    return EXIT_SUCCESS;
}
