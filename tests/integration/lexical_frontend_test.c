#include "../support/test.h"
#include "newlang/lexer.h"

#include <stdlib.h>
#include <string.h>

/* Test-only Linux linker wrapping; production has no allocator abstraction. */
static size_t fail_reallocation;
static size_t reallocation_count;
void *__real_realloc(void *pointer, size_t size);
void *__wrap_realloc(void *pointer, size_t size)
{
    if (fail_reallocation != 0 && ++reallocation_count == fail_reallocation) {
        return NULL;
    }
    return __real_realloc(pointer, size);
}

static bool file_pipeline(const char *path)
{
    static const char expected_bytes[] =
        "fn answer() -> i32 {\r\n    i32(42)\r\n}\r\n";
    static const NLToken expected[] = {
        {NL_TOKEN_WORD, {0, 2}},          {NL_TOKEN_WORD, {3, 9}},
        {NL_TOKEN_PUNCTUATION, {9, 10}},  {NL_TOKEN_PUNCTUATION, {10, 11}},
        {NL_TOKEN_PUNCTUATION, {12, 13}}, {NL_TOKEN_PUNCTUATION, {13, 14}},
        {NL_TOKEN_WORD, {15, 18}},        {NL_TOKEN_PUNCTUATION, {19, 20}},
        {NL_TOKEN_WORD, {26, 29}},        {NL_TOKEN_PUNCTUATION, {29, 30}},
        {NL_TOKEN_DIGITS, {30, 32}},      {NL_TOKEN_PUNCTUATION, {32, 33}},
        {NL_TOKEN_PUNCTUATION, {35, 36}}, {NL_TOKEN_EOF, {38, 38}}};
    NLSource *source = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK);
    CHECK(strcmp(nl_source_name(source), path) == 0);
    CHECK(nl_source_length(source) == sizeof(expected_bytes) - 1);
    NLSourceView original;
    CHECK(nl_source_view(source, (NLSourceSpan){0, nl_source_length(source)},
                         &original));
    CHECK(memcmp(original.bytes, expected_bytes, original.length) == 0);
    NLLexer lexer;
    CHECK(nl_lexer_init(&lexer, source));
    for (size_t repeat = 0; repeat < 2; ++repeat) {
        CHECK(nl_lexer_init(&lexer, source));
        for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i) {
            NLToken actual;
            const NLLexResult result = nl_lexer_next(&lexer, &actual);
            CHECK(result == (expected[i].kind == NL_TOKEN_EOF ? NL_LEX_EOF
                                                              : NL_LEX_TOKEN));
            CHECK(actual.kind == expected[i].kind);
            CHECK(actual.span.start_byte == expected[i].span.start_byte);
            CHECK(actual.span.end_byte == expected[i].span.end_byte);
            NLSourceView text;
            CHECK(nl_source_view(source, actual.span, &text));
            if (text.length != 0) {
                CHECK(memcmp(text.bytes,
                             expected_bytes + actual.span.start_byte,
                             text.length) == 0);
            }
        }
    }
    CHECK(memcmp(original.bytes, expected_bytes, original.length) == 0);
    CHECK(nl_source_load(path, &source) == NL_SOURCE_INVALID_ARGUMENT);
    nl_source_destroy(source);
    return true;
}

static bool binary_pipeline(const char *path)
{
    const unsigned char expected[] = {'a',  0,    'b',  '\r', '\n', 0xef,
                                      0xbb, 0xbf, 0xc3, 0xa9, 0xff, '\r'};
    NLSource *source = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK);
    CHECK(nl_source_length(source) == sizeof(expected));
    NLSourceView view;
    CHECK(nl_source_view(source, (NLSourceSpan){0, sizeof(expected)}, &view));
    CHECK(memcmp(view.bytes, expected, sizeof(expected)) == 0);
    NLLexer lexer;
    CHECK(nl_lexer_init(&lexer, source));
    NLToken token;
    CHECK(nl_lexer_next(&lexer, &token) == NL_LEX_TOKEN);
    CHECK(token.kind == NL_TOKEN_WORD && token.span.start_byte == 0 &&
          token.span.end_byte == 1);
    CHECK(nl_lexer_next(&lexer, &token) == NL_LEX_UNSUPPORTED);
    CHECK(token.kind == NL_TOKEN_UNSUPPORTED && token.span.start_byte == 1 &&
          token.span.end_byte == 2);
    CHECK(memcmp(view.bytes, expected, sizeof(expected)) == 0);
    nl_source_destroy(source);
    return true;
}

static bool empty_file(const char *path)
{
    NLSource *source = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK);
    CHECK(nl_source_length(source) == 0);
    NLLexer lexer;
    CHECK(nl_lexer_init(&lexer, source));
    NLToken token;
    CHECK(nl_lexer_next(&lexer, &token) == NL_LEX_EOF);
    CHECK(token.kind == NL_TOKEN_EOF && token.span.start_byte == 0 &&
          token.span.end_byte == 0);
    nl_source_destroy(source);
    return true;
}

static bool large_file_and_failures(const char *path, const char *missing,
                                    const char *directory)
{
    NLSource *source = NULL;
    CHECK(nl_source_load(missing, &source) == NL_SOURCE_IO_ERROR &&
          source == NULL);
    /* On the supported Linux host, a directory opens but fails binary reading.
     */
    CHECK(nl_source_load(directory, &source) == NL_SOURCE_IO_ERROR &&
          source == NULL);
    bool saw_failure = false;
    bool succeeded = false;
    for (size_t nth = 1; nth <= 16; ++nth) {
        fail_reallocation = nth;
        reallocation_count = 0;
        const NLSourceStatus status = nl_source_load(path, &source);
        fail_reallocation = 0;
        if (status == NL_SOURCE_OK) {
            succeeded = true;
            break;
        }
        CHECK(status == NL_SOURCE_OUT_OF_MEMORY && source == NULL);
        saw_failure = true;
    }
    CHECK(saw_failure && succeeded);
    CHECK(nl_source_length(source) == 15000);
    NLSourceView view;
    CHECK(nl_source_view(source, (NLSourceSpan){0, 15000}, &view));
    const unsigned char pattern[] = {'x', 0, '\r', '\n', 0xff};
    for (size_t i = 0; i < view.length; ++i) {
        CHECK(view.bytes[i] == pattern[i % sizeof(pattern)]);
    }
    nl_source_destroy(source);
    source = NULL;
    CHECK(nl_source_load(path, &source) ==
          NL_SOURCE_OK); /* Reusable after failure. */
    nl_source_destroy(source);
    return true;
}

static bool long_atom_file(const char *path)
{
    NLSource *source = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK);
    CHECK(nl_source_length(source) == 8192);
    NLLexer lexer;
    CHECK(nl_lexer_init(&lexer, source));
    NLToken token;
    CHECK(nl_lexer_next(&lexer, &token) == NL_LEX_TOKEN);
    CHECK(token.kind == NL_TOKEN_WORD && token.span.start_byte == 0 &&
          token.span.end_byte == 8192);
    NLSourceView view;
    CHECK(nl_source_view(source, token.span, &view));
    for (size_t i = 0; i < view.length; ++i) {
        CHECK(view.bytes[i] == 'z');
    }
    CHECK(nl_lexer_next(&lexer, &token) == NL_LEX_EOF);
    CHECK(token.kind == NL_TOKEN_EOF && token.span.start_byte == 8192 &&
          token.span.end_byte == 8192);
    nl_source_destroy(source);
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 8 || !file_pipeline(argv[1]) || !binary_pipeline(argv[2]) ||
        !empty_file(argv[3]) ||
        !large_file_and_failures(argv[4], argv[5], argv[6]) ||
        !long_atom_file(argv[7])) {
        return EXIT_FAILURE;
    }
    puts("source -> lexer: file bytes, exact spans, binary/empty files and "
         "I/O/OOM cleanup passed");
    return EXIT_SUCCESS;
}
