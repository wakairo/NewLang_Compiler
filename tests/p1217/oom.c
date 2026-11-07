/* Fault-inject only this invocation; old raw/native evidence is never rerun. */
#define main p1217_cli_main
#include "main.c"
#undef main
#include <assert.h>
#include <dirent.h>
typedef struct {
    NLSource *source, *entry_source;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *tree, *entry_tree;
    NLSemanticContext *world;
    NLCheckedFragment *root;
} OOMInput;
static bool enabled;
static size_t calls, fail_at;
static void *sites[16384];
static char kinds[16384];
static bool inject(char kind, void *pc)
{
    if (!enabled)
        return false;
    assert(calls < 16384);
    sites[calls] = pc;
    kinds[calls] = kind;
    return ++calls == fail_at;
}
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
void *__wrap_malloc(size_t n)
{
    return inject('m', __builtin_return_address(0)) ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t m)
{
    return inject('c', __builtin_return_address(0)) ? NULL
                                                    : __real_calloc(n, m);
}
void *__wrap_realloc(void *p, size_t n)
{
    return inject('r', __builtin_return_address(0)) ? NULL
                                                    : __real_realloc(p, n);
}
static void parse(OOMInput *a, const char *path)
{
    assert(nl_source_load(path, &a->source) == NL_SOURCE_OK);
    assert(nl_parser_create(a->source, &a->parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(a->parser, &a->tree, NULL) ==
           NL_PARSE_OK);
    assert(nl_semantic_create(&a->world) == NL_CHECK_OK);
    assert(nl_source_create("main()", 6, "P1217 OOM actual entry",
                            &a->entry_source) == NL_SOURCE_OK);
    assert(nl_parser_create(a->entry_source, &a->entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(a->entry_parser, &a->entry_tree,
                                               NULL) == NL_PARSE_OK);
}
static NLCheckStatus registration(OOMInput *a)
{
    const NLSyntaxTree *units[] = {a->tree};
    return nl_semantic_register_function_unit(a->world, units, 1, NULL);
}
static NLCheckStatus invocation(OOMInput *a)
{
    return nl_semantic_check_expression(a->world, a->entry_tree, &a->root,
                                        NULL);
}
static void dispose(OOMInput *a)
{
    nl_checked_destroy(a->root);
    nl_semantic_destroy(a->world);
    nl_syntax_tree_destroy(a->entry_tree);
    nl_parser_destroy(a->entry_parser);
    nl_source_destroy(a->entry_source);
    nl_syntax_tree_destroy(a->tree);
    nl_parser_destroy(a->parser);
    nl_source_destroy(a->source);
}
static void print_sites(const char *stage)
{
    for (size_t i = 0; i < calls; ++i)
        printf("P1217 SITE stage=%s ordinal=%zu kind=%c pc=%p\n", stage, i + 1,
               kinds[i], sites[i]);
}
static void start(size_t point)
{
    calls = 0;
    fail_at = point;
    enabled = true;
}
static void successful_emit(OOMInput *a)
{
    char *out = NULL;
    size_t n = 0;
    assert(nl_checked_c_node(a->root, &out, &n) == NL_NODE_C_OK && out && n);
    free(out);
}
static void census(const char *path)
{
    for (size_t stage = 0; stage < 3; ++stage) {
        OOMInput a = {0};
        parse(&a, path);
        if (stage)
            assert(registration(&a) == NL_CHECK_OK);
        if (stage == 2)
            assert(invocation(&a) == NL_CHECK_OK);
        start(SIZE_MAX);
        if (!stage)
            assert(registration(&a) == NL_CHECK_OK);
        else if (stage == 1)
            assert(invocation(&a) == NL_CHECK_OK);
        else
            successful_emit(&a);
        enabled = false;
        print_sites(stage == 0   ? "registration"
                    : stage == 1 ? "actual"
                                 : "emit");
        dispose(&a);
    }
}
static void trial(const char *path, const char *stage, size_t point)
{
    OOMInput a = {0};
    parse(&a, path);
    bool registered = strcmp(stage, "registration") != 0,
         emission = !strcmp(stage, "emit");
    if (registered)
        assert(registration(&a) == NL_CHECK_OK);
    if (emission)
        assert(invocation(&a) == NL_CHECK_OK);
    NLCopyCallAnchor anchor = {0};
    assert(nl_copy_state_anchor(a.world, &anchor) == NL_CHECK_OK);
    const NLSemanticContext before = *a.world;
    char *out = NULL;
    size_t length = 98765;
    start(point);
    int status = emission     ? (int)nl_checked_c_node(a.root, &out, &length)
                 : registered ? (int)invocation(&a)
                              : (int)registration(&a);
    enabled = false;
    assert(calls >= point);
    if (emission)
        assert(status == NL_NODE_C_OUT_OF_MEMORY && !out && length == 98765 &&
               a.root);
    else
        assert(status == NL_CHECK_OUT_OF_MEMORY && !a.root);
    assert(nl_copy_state_matches(a.world, &anchor));
    assert(a.world->types == before.types &&
           a.world->functions == before.functions &&
           a.world->type_count == before.type_count &&
           a.world->function_count == before.function_count &&
           a.world->last_incarnation == before.last_incarnation &&
           a.world->last_value_fact == before.last_value_fact);
    nl_copy_state_destroy(&anchor);
    if (!registered)
        assert(registration(&a) == NL_CHECK_OK);
    if (!emission)
        assert(invocation(&a) == NL_CHECK_OK);
    successful_emit(&a);
    dispose(&a);
    printf("P1217 REACHED stage=%s ordinal=%zu "
           "exact-OOM/rollback/no-publication/retry PASS\n",
           stage, point);
}
static void named(const char *source, size_t point)
{
    char directory[] = "/tmp/p1217-named-XXXXXX", target[128];
    assert(mkdtemp(directory));
    assert(snprintf(target, sizeof target, "%s/out.c", directory) > 0);
    FILE *f = fopen(target, "wb");
    assert(f && fputs("P1217 unchanged output\n", f) >= 0 && fclose(f) == 0);
    struct stat before, after;
    assert(stat(target, &before) == 0);
    start(point);
    int status = emit_c17_to(target, source);
    enabled = false;
    if (point == SIZE_MAX) {
        assert(status == 0);
        print_sites("named");
    } else {
        assert(status != 0 && calls >= point && stat(target, &after) == 0);
        assert(before.st_ino == after.st_ino &&
               before.st_mtim.tv_sec == after.st_mtim.tv_sec &&
               before.st_mtim.tv_nsec == after.st_mtim.tv_nsec);
        char buffer[64] = {0};
        f = fopen(target, "rb");
        assert(f &&
               fread(buffer, 1, sizeof buffer, f) ==
                   strlen("P1217 unchanged output\n") &&
               fclose(f) == 0 && !strcmp(buffer, "P1217 unchanged output\n"));
        DIR *d = opendir(directory);
        assert(d);
        for (struct dirent *e = readdir(d); e; e = readdir(d))
            assert(!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..") ||
                   !strcmp(e->d_name, "out.c"));
        assert(closedir(d) == 0);
        assert(emit_c17_to(target, source) == 0);
        printf("P1217 REACHED stage=named ordinal=%zu "
               "reject/bytes-inode-mtime/no-temp/retry PASS\n",
               point);
    }
    assert(unlink(target) == 0 && rmdir(directory) == 0);
}
int main(int argc, char **argv)
{
    assert(argc >= 3);
    if (!strcmp(argv[1], "census")) {
        assert(argc == 3);
        census(argv[2]);
        named(argv[2], SIZE_MAX);
        return 0;
    }
    assert(argc == 4);
    size_t point = (size_t)strtoull(argv[3], NULL, 10);
    assert(point);
    if (!strcmp(argv[1], "named"))
        named(argv[2], point);
    else
        trial(argv[2], argv[1], point);
    return 0;
}
