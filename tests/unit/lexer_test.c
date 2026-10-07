#include "../support/test.h"
#include "newlang/lexer.h"

#include <stdlib.h>
#include <string.h>

static bool token_equal(NLToken left, NLToken right)
{
    return left.kind == right.kind &&
           left.span.start_byte == right.span.start_byte &&
           left.span.end_byte == right.span.end_byte;
}

static bool sequence(const void *bytes, size_t length, const NLToken *expected,
                     size_t count)
{
    NLSource *source = NULL;
    CHECK(nl_source_create(bytes, length, "lexer-unit", &source) ==
          NL_SOURCE_OK);
    NLLexer first;
    NLLexer second;
    CHECK(nl_lexer_init(&first, source) && nl_lexer_init(&second, source));
    NLSourceView original;
    CHECK(nl_source_view(source, (NLSourceSpan){0, length}, &original));
    for (size_t i = 0; i < count; ++i) {
        NLToken a;
        NLToken b;
        const NLLexResult status =
            expected[i].kind == NL_TOKEN_EOF ? NL_LEX_EOF : NL_LEX_TOKEN;
        CHECK(nl_lexer_next(&first, &a) == status);
        CHECK(nl_lexer_next(&second, &b) == status);
        CHECK(token_equal(a, expected[i]) && token_equal(a, b));
        NLSourceView text;
        CHECK(nl_source_view(source, a.span, &text));
        CHECK(text.length == a.span.end_byte - a.span.start_byte);
        if (text.length != 0) {
            CHECK(memcmp(text.bytes,
                         (const unsigned char *)bytes + a.span.start_byte,
                         text.length) == 0);
        }
    }
    NLToken eof;
    CHECK(nl_lexer_next(&first, &eof) == NL_LEX_EOF);
    CHECK(token_equal(eof, (NLToken){NL_TOKEN_EOF, {length, length}}));
    CHECK(nl_lexer_next(&first, &eof) == NL_LEX_EOF);
    if (length != 0) {
        CHECK(memcmp(original.bytes, bytes, length) == 0);
    }
    CHECK(nl_lexer_init(&first, source));
    CHECK(nl_lexer_next(&first, &eof) ==
          (expected[0].kind == NL_TOKEN_EOF ? NL_LEX_EOF : NL_LEX_TOKEN));
    CHECK(token_equal(eof, expected[0]));
    nl_source_destroy(source); /* Lexers require no deinit/resources. */
    return true;
}

static bool supported_sequences(void)
{
    const NLToken empty[] = {{NL_TOKEN_EOF, {0, 0}}};
    CHECK(sequence(NULL, 0, empty, 1));
    const NLToken spacing[] = {{NL_TOKEN_EOF, {5, 5}}};
    CHECK(sequence(" \t\n\r\n", 5, spacing, 1));
    const NLToken word[] = {{NL_TOKEN_WORD, {0, 6}}, {NL_TOKEN_EOF, {6, 6}}};
    CHECK(sequence("_Abc09", 6, word, 2));
    const NLToken digits[] = {{NL_TOKEN_DIGITS, {0, 5}},
                              {NL_TOKEN_EOF, {5, 5}}};
    CHECK(sequence("00123", 5, digits, 2));
    const NLToken mixed[] = {
        {NL_TOKEN_WORD, {1, 3}},          {NL_TOKEN_WORD, {4, 5}},
        {NL_TOKEN_PUNCTUATION, {5, 6}},   {NL_TOKEN_PUNCTUATION, {6, 7}},
        {NL_TOKEN_WORD, {7, 10}},         {NL_TOKEN_PUNCTUATION, {10, 11}},
        {NL_TOKEN_PUNCTUATION, {11, 12}}, {NL_TOKEN_DIGITS, {12, 13}},
        {NL_TOKEN_PUNCTUATION, {13, 14}}, {NL_TOKEN_EOF, {16, 16}}};
    CHECK(sequence(" fn x->i32(-2)\r\n", 16, mixed,
                   sizeof(mixed) / sizeof(mixed[0])));
    const char *const marks = "(){}[],:;.@+-*/%<>=!&|?";
    NLToken expected[32];
    const size_t length = strlen(marks);
    CHECK(length + 1 <= sizeof(expected) / sizeof(expected[0]));
    for (size_t i = 0; i < length; ++i) {
        expected[i] = (NLToken){NL_TOKEN_PUNCTUATION, {i, i + 1}};
    }
    expected[length] = (NLToken){NL_TOKEN_EOF, {length, length}};
    CHECK(sequence(marks, length, expected, length + 1));
    const NLToken composite[] = {
        {NL_TOKEN_PUNCTUATION, {0, 1}}, {NL_TOKEN_PUNCTUATION, {1, 2}},
        {NL_TOKEN_PUNCTUATION, {2, 3}}, {NL_TOKEN_PUNCTUATION, {3, 4}},
        {NL_TOKEN_PUNCTUATION, {4, 5}}, {NL_TOKEN_PUNCTUATION, {5, 6}},
        {NL_TOKEN_PUNCTUATION, {6, 7}}, {NL_TOKEN_PUNCTUATION, {7, 8}},
        {NL_TOKEN_EOF, {8, 8}}};
    CHECK(sequence("->==&&||", 8, composite, 9));
    const NLToken boundary[] = {{NL_TOKEN_DIGITS, {0, 3}},
                                {NL_TOKEN_WORD, {3, 6}},
                                {NL_TOKEN_EOF, {6, 6}}};
    CHECK(sequence("123abc", 6, boundary, 3));
    const NLToken words[] = {
        {NL_TOKEN_WORD, {0, 2}},   {NL_TOKEN_WORD, {3, 6}},
        {NL_TOKEN_WORD, {7, 13}},  {NL_TOKEN_WORD, {14, 16}},
        {NL_TOKEN_WORD, {17, 21}}, {NL_TOKEN_EOF, {21, 21}}};
    CHECK(sequence("fn let return if else", 21, words, 6));
    return true;
}

static bool unsupported_bytes(void)
{
    /* Every byte is covered without depending on ctype or locale. */
    for (unsigned int value = 0; value <= 255; ++value) {
        const unsigned char byte = (unsigned char)value;
        if ((value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') ||
            (value >= '0' && value <= '9') || value == '_' || value == ' ' ||
            value == '\t' || value == '\n' ||
            (value != 0 &&
             strchr("(){}[],:;.@+-*/%<>=!&|?", (int)value) != NULL)) {
            continue;
        }
        unsigned char input[] = {'a', ' ', byte, 'z'};
        NLSource *source = NULL;
        CHECK(nl_source_create(input, sizeof(input), "unsupported", &source) ==
              NL_SOURCE_OK);
        NLLexer lexer;
        CHECK(nl_lexer_init(&lexer, source));
        NLToken token;
        CHECK(nl_lexer_next(&lexer, &token) == NL_LEX_TOKEN);
        CHECK(token_equal(token, (NLToken){NL_TOKEN_WORD, {0, 1}}));
        for (size_t repeat = 0; repeat < 3; ++repeat) {
            CHECK(nl_lexer_next(&lexer, &token) == NL_LEX_UNSUPPORTED);
            CHECK(token_equal(token, (NLToken){NL_TOKEN_UNSUPPORTED, {2, 3}}));
        }
        NLSourceView view;
        CHECK(nl_source_view(source, (NLSourceSpan){0, sizeof(input)}, &view));
        CHECK(memcmp(input, view.bytes, sizeof(input)) == 0);
        CHECK(nl_lexer_init(&lexer, source));
        CHECK(nl_lexer_next(&lexer, &token) == NL_LEX_TOKEN);
        nl_source_destroy(source);
    }
    const char *const openers[] = {" //body", " /*body*/"};
    for (size_t i = 0; i < 2; ++i) {
        NLSource *source = NULL;
        CHECK(nl_source_create(openers[i], strlen(openers[i]), "comment",
                               &source) == NL_SOURCE_OK);
        NLLexer lexer;
        CHECK(nl_lexer_init(&lexer, source));
        NLToken token;
        CHECK(nl_lexer_next(&lexer, &token) == NL_LEX_UNSUPPORTED);
        CHECK(token_equal(token, (NLToken){NL_TOKEN_UNSUPPORTED, {1, 3}}));
        CHECK(nl_lexer_next(&lexer, &token) == NL_LEX_UNSUPPORTED);
        CHECK(token_equal(token, (NLToken){NL_TOKEN_UNSUPPORTED, {1, 3}}));
        nl_source_destroy(source);
    }
    return true;
}

static bool internal_failure(void)
{
    NLLexer lexer;
    CHECK(!nl_lexer_init(NULL, NULL));
    CHECK(!nl_lexer_init(&lexer, NULL));
    NLToken token = {NL_TOKEN_DIGITS, {99, 100}};
    CHECK(nl_lexer_next(&lexer, &token) == NL_LEX_INTERNAL_ERROR);
    CHECK(nl_lexer_next(NULL, &token) == NL_LEX_INTERNAL_ERROR);
    CHECK(token_equal(token, (NLToken){NL_TOKEN_DIGITS, {99, 100}}));
    NLSource *source = NULL;
    CHECK(nl_source_create("word", 4, "live", &source) == NL_SOURCE_OK);
    CHECK(nl_lexer_init(&lexer, source));
    CHECK(nl_lexer_next(&lexer, NULL) == NL_LEX_INTERNAL_ERROR);
    CHECK(nl_lexer_next(&lexer, &token) == NL_LEX_TOKEN);
    CHECK(token_equal(token, (NLToken){NL_TOKEN_WORD, {0, 4}}));
    nl_source_destroy(source);
    return true;
}

int main(void)
{
    if (!supported_sequences() || !unsupported_bytes() || !internal_failure()) {
        return EXIT_FAILURE;
    }
    puts("lexer: exact spans, deterministic sequences, EOF and unsupported "
         "states passed");
    return EXIT_SUCCESS;
}
