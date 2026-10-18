#include "../support/node_checked.h"
#include "newlang/usize_addition.h"
#include <stdlib.h>

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t index_, fail_at;
void *__wrap_malloc(size_t n)
{
    return injecting && index_++ == fail_at ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return injecting && index_++ == fail_at ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return injecting && index_++ == fail_at ? NULL : __real_realloc(p, n);
}
static bool exercise(const char *path)
{
    NLSource *source = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK);
    NLParser *parser = NULL;
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    for (fail_at = 0; fail_at < 1000; ++fail_at) {
        NLSyntaxTree *tree = NULL;
        index_ = 0;
        injecting = true;
        NLParseStatus s = nl_parser_parse_function_unit(parser, &tree, NULL);
        injecting = false;
        if (s == NL_PARSE_OK) {
            nl_syntax_tree_destroy(tree);
            break;
        }
        CHECK(s == NL_PARSE_OUT_OF_MEMORY && tree == NULL);
    }
    CHECK(fail_at > 0 && fail_at < 1000);
    printf("parser-oom=%zu; partial-tree=NULL; reusable-parser=PASS\n",
           fail_at);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    size_t id = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i)
        if (nl_checked_node_view(f, i)->kind == NL_CHECKED_USIZE_ADDITION)
            id = i;
    CHECK(id);
    fail_at = 0;
    index_ = 0;
    injecting = true;
    const bool valid = nl_checked_usize_addition_valid(f, id, NULL);
    injecting = false;
    CHECK(valid && index_ == 0);
    printf("public-validator-allocations=0\n");
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && exercise(argv[1]) ? 0 : 1;
}
