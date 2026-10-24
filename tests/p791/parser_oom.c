#include "../support/node_checked.h"
#include <stdlib.h>
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool active;
static size_t at, calls;
void *__wrap_malloc(size_t n)
{
    return active && calls++ == at ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return active && calls++ == at ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return active && calls++ == at ? NULL : __real_realloc(p, n);
}
static bool run(int argc, char **argv)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    CHECK(argc == 2 && nl_source_load(argv[1], &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK);
    for (at = 0; at < 1000; ++at) {
        NLSyntaxTree *tree = NULL;
        calls = 0;
        active = true;
        NLParseStatus status =
            nl_parser_parse_function_unit(parser, &tree, NULL);
        active = false;
        if (status == NL_PARSE_OK) {
            CHECK(tree);
            nl_syntax_tree_destroy(tree);
            break;
        }
        CHECK(status == NL_PARSE_OUT_OF_MEMORY && !tree);
        CHECK(nl_parser_parse_function_unit(parser, &tree, NULL) ==
              NL_PARSE_OK);
        nl_syntax_tree_destroy(tree);
    }
    CHECK(at > 0 && at < 1000);
    printf("P791 parser reached OOM positions=%zu clean/reuse=PASS\n", at);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}
int main(int argc, char **argv)
{
    return run(argc, argv) ? EXIT_SUCCESS : EXIT_FAILURE;
}
