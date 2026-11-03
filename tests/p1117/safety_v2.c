#define main p1117_driver_main
#include "main.c"
#undef main
#include <assert.h>
#include <dirent.h>

typedef struct {
    NLSource *source, *entry_source;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *unit, *entry_tree;
    NLSemanticContext *context;
    NLCheckedFragment *root;
    NLCheckedFragment *owner, *helper;
    NLCheckedNodeId call, choice, comparison;
} Input;
static void dispose(Input *i)
{
    nl_checked_destroy(i->root);
    nl_semantic_destroy(i->context);
    nl_syntax_tree_destroy(i->entry_tree);
    nl_parser_destroy(i->entry_parser);
    nl_source_destroy(i->entry_source);
    nl_syntax_tree_destroy(i->unit);
    nl_parser_destroy(i->parser);
    nl_source_destroy(i->source);
    *i = (Input){0};
}
static void syntax(const char *path, Input *i)
{
    assert(nl_source_load(path, &i->source) == NL_SOURCE_OK);
    assert(nl_parser_create(i->source, &i->parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(i->parser, &i->unit, NULL) ==
           NL_PARSE_OK);
    assert(nl_source_create("main()", 6, "<P1117-entry>", &i->entry_source) ==
           NL_SOURCE_OK);
    assert(nl_parser_create(i->entry_source, &i->entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(i->entry_parser, &i->entry_tree,
                                               NULL) == NL_PARSE_OK);
}
static NLCheckStatus registration(Input *i)
{
    const NLSyntaxTree *units[] = {i->unit};
    return nl_semantic_register_function_unit(i->context, units, 1, NULL);
}
static NLCheckStatus invocation(Input *i)
{
    return nl_semantic_check_expression(i->context, i->entry_tree, &i->root,
                                        NULL);
}
static bool current(const Input *i)
{
    V0Program p = {.unit = nl_semantic_unit_type(i->context),
                   .u8 = nl_semantic_core_type(i->context, NL_TYPE_U8)};
    return i->root && v0_validate_call(i->root, nl_checked_root(i->root), &p);
}
static void prepare(const char *path, Input *i)
{
    syntax(path, i);
    assert(nl_semantic_create(&i->context) == NL_CHECK_OK);
    assert(registration(i) == NL_CHECK_OK);
    assert(invocation(i) == NL_CHECK_OK && current(i));
    i->owner = (NLCheckedFragment *)nl_checked_call_body(
        i->root, nl_checked_root(i->root));
    assert(i->owner);
    for (size_t n = 1; n <= i->owner->count; ++n)
        if (i->owner->nodes[n - 1].kind == NL_CHECKED_REGISTERED_CALL) {
            i->call = n;
            break;
        }
    assert(i->call);
    i->helper = (NLCheckedFragment *)nl_checked_call_body(i->owner, i->call);
    assert(i->helper);
    for (size_t n = 1; n <= i->helper->count; ++n) {
        NLCheckedNodeView *v = &i->helper->nodes[n - 1];
        if (v->kind == NL_CHECKED_IF)
            i->choice = n;
        if (v->kind == NL_CHECKED_USIZE_EQUALITY ||
            v->kind == NL_CHECKED_USIZE_LESS_THAN)
            i->comparison = n;
    }
    assert(i->choice && i->comparison);
}

#ifndef P1117_OOM
static size_t rejects;
static void refused(Input *i, const char *name)
{
    assert(!current(i));
    printf("P1117 current refusal %zu %s\n", ++rejects, name);
}
static void dependencies(void)
{
    for (int kind = NL_DEPENDENCY_FREE; kind <= NL_DEPENDENCIES_UNKNOWN;
         ++kind) {
        NLSemanticContext *c = NULL;
        assert(nl_semantic_create(&c) == NL_CHECK_OK);
        const NLTypeId q = nl_semantic_core_type(c, NL_TYPE_USIZE);
        NLSymbolId symbol;
        assert(nl_semantic_seed_value(c, "quantity", q,
                                      (NLDependencyKnowledge)kind,
                                      &symbol) == NL_CHECK_OK);
        for (size_t less = 0; less < 2; ++less) {
            NLSource *source = NULL;
            NLParser *parser = NULL;
            NLSyntaxTree *tree = NULL;
            NLCheckedFragment *out = NULL;
            const char *text =
                less ? "quantity < quantity" : "quantity == quantity";
            assert(nl_source_create(text, strlen(text),
                                    "P1117-fixture-only-dependency",
                                    &source) == NL_SOURCE_OK);
            assert(nl_parser_create(source, &parser) == NL_PARSE_OK);
            assert(nl_parser_parse_source_fragment(parser, &tree, NULL) ==
                   NL_PARSE_OK);
            void *before = malloc(sizeof(*c));
            assert(before);
            memcpy(before, c, sizeof(*c));
            NLCheckStatus s = nl_semantic_check_source(c, tree, &out, NULL);
            if (kind == NL_DEPENDENCY_FREE) {
                assert(s == NL_CHECK_OK && out);
                const NLCheckedNodeView *v =
                    nl_checked_node_view(out, nl_checked_root(out));
                NLSemanticValueView value;
                assert(v && v->has_scalar_result && !v->scalar_result.known &&
                       nl_semantic_value_view(c, v->results[0].value, &value) &&
                       !value.scalar_known && !value.scalar_value &&
                       value.dependencies == NL_DEPENDENCY_FREE);
            } else {
                assert(s == NL_CHECK_ANALYSIS_PRECISION_LIMIT && !out);
                assert(!memcmp(before, c, sizeof(*c)));
            }
            printf("P1117 fixture-only unknown predicate dependency=%d "
                   "less=%zu status=%d\n",
                   kind, less, s);
            free(before);
            nl_checked_destroy(out);
            nl_syntax_tree_destroy(tree);
            nl_parser_destroy(parser);
            nl_source_destroy(source);
        }
        nl_semantic_destroy(c);
    }
}
static void negatives(const char *path)
{
    Input a = {0}, b = {0};
    prepare(path, &a);
    prepare(path, &b);
    NLCheckedNodeView *call = &a.owner->nodes[a.call - 1];
    NLFunctionEntry *fn = &a.context->functions[call->function - 1];
    NLFunctionEntry entry = *fn;
    fn->body = NULL;
    refused(&a, "NULL body");
    *fn = entry;
    fn->kind = NL_CHECKED_U8_LITERAL;
    refused(&a, "wrong callable kind");
    *fn = entry;
    fn->result = nl_semantic_unit_type(a.context);
    refused(&a, "wrong current result");
    *fn = entry;
    fn->hidden_dependencies = true;
    refused(&a, "current hidden dependency");
    *fn = entry;
    fn->caller_effects = true;
    refused(&a, "current effects");
    *fn = entry;
    fn->owner_receiver = true;
    refused(&a, "current owner kind");
    *fn = entry;
    fn->parameters = NULL;
    refused(&a, "NULL signature");
    *fn = entry;
    fn->name = "foreign_function";
    refused(&a, "wrong current source name");
    *fn = entry;
    NLTypeId type = fn->parameters[0];
    fn->parameters[0] = nl_semantic_core_type(a.context, NL_TYPE_U8);
    refused(&a, "u8 signature cannot truncate usize");
    fn->parameters[0] = type;
    NLSyntaxTree *tree = fn->body->syntax;
    fn->body->syntax = NULL;
    refused(&a, "NULL current plan syntax");
    fn->body->syntax = tree;
    const NLSource *source = a.helper->source;
    a.helper->source = b.helper->source;
    refused(&a, "foreign source");
    a.helper->source = source;
    const NLSemanticContext *world = a.helper->context;
    a.helper->context = b.context;
    refused(&a, "foreign world");
    a.helper->context = world;
    NLFunctionBody *plan = a.helper->body_owner;
    a.helper->body_owner = b.helper->body_owner;
    refused(&a, "foreign plan owner");
    a.helper->body_owner = plan;
    NLFunctionEntry *functions = a.context->functions;
    a.context->functions = NULL;
    refused(&a, "NULL functions");
    a.context->functions = functions;
    size_t count = a.context->function_count;
    a.context->function_count = 0;
    refused(&a, "function index outside current count");
    a.context->function_count = count;
    NLCheckedNodeView saved = *call;
    call->function = SIZE_MAX;
    refused(&a, "function index overflow");
    *call = saved;
    NLCheckedNodeView *comparison = &a.helper->nodes[a.comparison - 1];
    saved = *comparison;
    comparison->kind = saved.kind == NL_CHECKED_USIZE_EQUALITY
                           ? NL_CHECKED_USIZE_LESS_THAN
                           : NL_CHECKED_USIZE_EQUALITY;
    refused(&a, "changed operator");
    *comparison = saved;
    comparison->results[0].value = 0;
    refused(&a, "unissued scalar result");
    *comparison = saved;
    comparison->scalar_result.known = !comparison->scalar_result.known;
    refused(&a, "forged truth knowledge");
    *comparison = saved;
    count = a.helper->count;
    a.helper->count = SIZE_MAX;
    refused(&a, "node count overflow");
    a.helper->count = count;
    assert(current(&a) && current(&b));
    a.helper->arms = b.helper->arms;
    a.helper->arm_count = b.helper->arm_count;
    refused(&a, "foreign whole IF arm display");
    dispose(&a); /* Intentionally unrestored display; canonical custody only. */
    assert(current(&b));
    dispose(&b);
    puts("P1117 canonical A destruction preserves genuine B");
    dependencies();
}
#else
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t allocation, failure;
void *__wrap_malloc(size_t n)
{
    return injecting && allocation++ == failure ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t size)
{
    return injecting && allocation++ == failure ? NULL : __real_calloc(n, size);
}
void *__wrap_realloc(void *p, size_t n)
{
    return injecting && allocation++ == failure ? NULL : __real_realloc(p, n);
}
static void api_oom(const char *path, bool invoke)
{
    Input i = {0};
    syntax(path, &i);
    size_t reached = 0;
    for (size_t fail = SIZE_MAX;; fail = fail == SIZE_MAX ? 0 : fail + 1) {
        assert(nl_semantic_create(&i.context) == NL_CHECK_OK);
        if (invoke)
            assert(registration(&i) == NL_CHECK_OK);
        void *before = malloc(sizeof(*i.context));
        assert(before);
        memcpy(before, i.context, sizeof(*i.context));
        allocation = 0;
        failure = fail;
        injecting = true;
        NLCheckStatus s = invoke ? invocation(&i) : registration(&i);
        injecting = false;
        if (fail == SIZE_MAX) {
            assert(s == NL_CHECK_OK);
            reached = allocation;
            assert(reached);
        } else {
            assert(allocation > fail && s == NL_CHECK_OUT_OF_MEMORY);
            assert(!i.root && !memcmp(before, i.context, sizeof(*i.context)));
            assert((invoke ? invocation(&i) : registration(&i)) == NL_CHECK_OK);
        }
        if (!invoke)
            assert(invocation(&i) == NL_CHECK_OK);
        assert(current(&i));
        free(before);
        nl_checked_destroy(i.root);
        i.root = NULL;
        nl_semantic_destroy(i.context);
        i.context = NULL;
        if (fail != SIZE_MAX && fail + 1 == reached)
            break;
    }
    dispose(&i);
    printf("P1117 actual %s OOM locations=%zu rollback/no-publication/retry "
           "PASS\n",
           invoke ? "invocation" : "registration", reached);
}
static void sentinel(const char *path)
{
    FILE *f = fopen(path, "wb");
    assert(f && fputs("P1117 original output\n", f) >= 0 && !fclose(f));
}
static void cli_oom(const char *path)
{
    char directory[] = "/tmp/p1117-named-XXXXXX";
    assert(mkdtemp(directory));
    char output[128];
    assert(snprintf(output, sizeof(output), "%s/out.c", directory) > 0);
    char *args[] = {"newlangc", "--emit-c17-to", output, (char *)path};
    size_t reached = 0;
    for (size_t fail = SIZE_MAX;; fail = fail == SIZE_MAX ? 0 : fail + 1) {
        sentinel(output);
        struct stat before, after;
        assert(!stat(output, &before));
        allocation = 0;
        failure = fail;
        injecting = true;
        const int s = p1117_driver_main(4, args);
        injecting = false;
        if (fail == SIZE_MAX) {
            assert(!s);
            reached = allocation;
            assert(reached);
        } else {
            assert(s == 1 && allocation > fail && !stat(output, &after));
            assert(before.st_ino == after.st_ino &&
                   before.st_size == after.st_size &&
                   before.st_mtim.tv_sec == after.st_mtim.tv_sec &&
                   before.st_mtim.tv_nsec == after.st_mtim.tv_nsec);
            FILE *f = fopen(output, "rb");
            char bytes[64] = {0};
            assert(f && fread(bytes, 1, sizeof(bytes), f) == 22 && !fclose(f));
            assert(!memcmp(bytes, "P1117 original output\n", 22));
            DIR *d = opendir(directory);
            assert(d);
            size_t files = 0;
            struct dirent *e;
            while ((e = readdir(d)))
                if (strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) {
                    assert(!strcmp(e->d_name, "out.c"));
                    ++files;
                }
            assert(files == 1 && !closedir(d));
            assert(!p1117_driver_main(4, args));
        }
        if (fail != SIZE_MAX && fail + 1 == reached)
            break;
    }
    assert(!unlink(output) && !rmdir(directory));
    printf("P1117 actual named CLI OOM locations=%zu bytes/inode/mtime/temp/"
           "retry PASS\n",
           reached);
}
#endif
int main(int argc, char **argv)
{
    assert(argc == 2);
#ifdef P1117_OOM
    /* The normal owned setup is also validated by the OOM binary. */
    Input i = {0};
    prepare(argv[1], &i);
    dispose(&i);
    api_oom(argv[1], false);
    api_oom(argv[1], true);
    cli_oom(argv[1]);
#else
    negatives(argv[1]);
#endif
    return 0;
}
