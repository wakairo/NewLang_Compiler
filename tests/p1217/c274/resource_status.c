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

static const NLCheckedFragment *seen[256], *issuer;
static NLCheckedNodeId producer;
static size_t seen_count;
static void find_producer(const NLCheckedFragment *f)
{
    if (!f)
        return;
    for (size_t i = 0; i < seen_count; ++i)
        if (seen[i] == f)
            return;
    assert(seen_count < 256);
    seen[seen_count++] = f;
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        assert(n);
        if (n->producer.entry_proved) {
            assert(!issuer);
            issuer = f;
            producer = id;
        }
        find_producer(nl_checked_call_body(f, id));
        if (n->kind == NL_CHECKED_MATCH)
            for (size_t arm = 0; arm < n->item_count; ++arm)
                find_producer(nl_checked_match_arm(f, id, arm));
        if (n->kind == NL_CHECKED_IF)
            for (size_t arm = 0; arm < 2; ++arm)
                find_producer(nl_checked_if_arm(f, id, arm));
    }
}
int main(int argc, char **argv)
{
    assert(argc == 3);
    setvbuf(stdout, NULL, _IONBF, 0);
    OOMInput a = {0};
    parse(&a, argv[1]);
    assert(registration(&a) == NL_CHECK_OK && invocation(&a) == NL_CHECK_OK);
    find_producer(a.root);
    assert(issuer && producer);
    assert(nl_checked_producer_valid(issuer, producer));
    successful_emit(&a);
    NLCopyCallAnchor anchor = {0};
    assert(nl_copy_state_anchor(a.world, &anchor) == NL_CHECK_OK);
    NLCheckedFragment *root_before = a.root;
    start(1);
    bool valid = nl_checked_producer_valid(issuer, producer);
    enabled = false;
    printf("C274 public-bool scratch-OOM=%d calls=%zu root-same=%d\n", valid,
           calls, a.root == root_before);
    print_sites("public-bool");
    assert(!valid && calls >= 1 && a.root == root_before &&
           nl_copy_state_matches(a.world, &anchor));
    assert(nl_checked_producer_valid(issuer, producer));
    char *out = NULL;
    size_t length = 98765;
    start(2);
    NLNodeCStatus status = nl_checked_c_node(a.root, &out, &length);
    enabled = false;
    printf("C274 emit ordinal=2 status=%d out-null=%d length=%zu root-same=%d "
           "calls=%zu\n",
           status, !out, length, a.root == root_before, calls);
    print_sites("emit");
    assert(status == (NLNodeCStatus)atoi(argv[2]) && !out && length == 98765 &&
           a.root == root_before && calls >= 2);
    assert(nl_copy_state_matches(a.world, &anchor));
    assert(nl_checked_producer_valid(issuer, producer));
    successful_emit(&a);
    NLCheckedNodeView *v =
        (NLCheckedNodeView *)nl_checked_node_view(issuer, producer);
    bool saved = v->producer.definition.head_link_required;
    v->producer.definition.head_link_required = false;
    assert(!nl_checked_producer_valid(issuer, producer));
    out = NULL;
    length = 98765;
    status = nl_checked_c_node(a.root, &out, &length);
    printf("C274 logical-invalid producer status=%d out-null=%d length=%zu "
           "root-same=%d\n",
           status, !out, length, a.root == root_before);
    assert(status == NL_NODE_C_UNSUPPORTED && !out && length == 98765 &&
           a.root == root_before);
    v->producer.definition.head_link_required = saved;
    assert(nl_copy_state_matches(a.world, &anchor));
    nl_copy_state_destroy(&anchor);
    successful_emit(&a);
    dispose(&a);
    puts("C274 rollback/retry/canonical-destroy PASS");
    return 0;
}
