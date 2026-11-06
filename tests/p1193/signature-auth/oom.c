/* Focused OOM successor for the one added signature-ledger allocation.
 * Registration, named output and all older allocation sites remain covered by
 * the preserved P1193 gates. */
#include "semantic_check.c"
#include <assert.h>

typedef struct {
    NLSource *source, *entry;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *syntax, *entry_syntax;
    NLSemanticContext *world;
    NLCheckedFragment *root;
} AuthInput;

static void auth_parse(AuthInput *a, const char *path)
{
    assert(nl_source_load(path, &a->source) == NL_SOURCE_OK);
    assert(nl_parser_create(a->source, &a->parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(a->parser, &a->syntax, NULL) ==
           NL_PARSE_OK);
    assert(nl_source_create("main()", 6, "F1198 focused OOM", &a->entry) ==
           NL_SOURCE_OK);
    assert(nl_parser_create(a->entry, &a->entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(
               a->entry_parser, &a->entry_syntax, NULL) == NL_PARSE_OK);
    assert(nl_semantic_create(&a->world) == NL_CHECK_OK);
}

static void auth_register(AuthInput *a)
{
    const NLSyntaxTree *units[] = {a->syntax};
    assert(nl_semantic_register_function_unit(a->world, units, 1, NULL) ==
           NL_CHECK_OK);
}

static void auth_dispose(AuthInput *a)
{
    nl_checked_destroy(a->root);
    nl_semantic_destroy(a->world);
    nl_syntax_tree_destroy(a->entry_syntax);
    nl_parser_destroy(a->entry_parser);
    nl_source_destroy(a->entry);
    nl_syntax_tree_destroy(a->syntax);
    nl_parser_destroy(a->parser);
    nl_source_destroy(a->source);
}

static NLCurrentLoopProof *auth_proof(AuthInput *a)
{
    NLCheckedFragment *body = (NLCheckedFragment *)nl_checked_call_body(
        a->root, nl_checked_root(a->root));
    assert(body && body->linear_breaks && body->linear_breaks->current_cycle);
    NLLinearBreakView view;
    assert(nl_linear_break_validate(body, body->linear_breaks->loop, &view) ==
               NL_CHECK_OK &&
           view.changing_current && view.scalar_loop && view.incoming_call);
    return body->linear_breaks->current_cycle;
}

static bool enabled;
static size_t calls, fail_at;
static void *returned[8192];
static size_t allocation_bytes[8192];
static char allocation_kinds[8192];

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);

static bool auth_fail(char kind, size_t bytes)
{
    assert(calls < sizeof(returned) / sizeof(*returned));
    allocation_kinds[calls] = kind;
    allocation_bytes[calls] = bytes;
    returned[calls] = NULL;
    ++calls;
    return calls == fail_at;
}

void *__wrap_malloc(size_t bytes)
{
    if (!enabled)
        return __real_malloc(bytes);
    if (auth_fail('m', bytes))
        return NULL;
    return returned[calls - 1] = __real_malloc(bytes);
}

void *__wrap_calloc(size_t count, size_t bytes)
{
    assert(!bytes || count <= SIZE_MAX / bytes);
    if (!enabled)
        return __real_calloc(count, bytes);
    if (auth_fail('c', count * bytes))
        return NULL;
    return returned[calls - 1] = __real_calloc(count, bytes);
}

void *__wrap_realloc(void *old, size_t bytes)
{
    if (!enabled)
        return __real_realloc(old, bytes);
    if (auth_fail('r', bytes))
        return NULL;
    return returned[calls - 1] = __real_realloc(old, bytes);
}

static size_t auth_census(const char *path)
{
    AuthInput a = {0};
    auth_parse(&a, path);
    auth_register(&a);
    calls = 0;
    fail_at = SIZE_MAX;
    enabled = true;
    assert(nl_semantic_check_expression(a.world, a.entry_syntax, &a.root,
                                        NULL) == NL_CHECK_OK);
    enabled = false;
    NLCurrentLoopProof *proof = auth_proof(&a);
    size_t ordinal = 0;
    for (size_t i = 0; i < calls; ++i)
        if (returned[i] == proof->signature_entries) {
            assert(!ordinal);
            ordinal = i + 1;
        }
    assert(ordinal && allocation_kinds[ordinal - 1] == 'c' &&
           allocation_bytes[ordinal - 1] ==
               proof->signature_entry_count *
                   sizeof(*proof->signature_entries));
    printf("AUTH OOM SITE ordinal=%zu/%zu kind=%c bytes=%zu entries=%zu\n",
           ordinal, calls, allocation_kinds[ordinal - 1],
           allocation_bytes[ordinal - 1], proof->signature_entry_count);
    auth_dispose(&a);
    return ordinal;
}

static void auth_oom(const char *path, size_t ordinal)
{
    AuthInput a = {0};
    auth_parse(&a, path);
    auth_register(&a);
    NLCopyCallAnchor before = {0};
    assert(nl_copy_state_anchor(a.world, &before) == NL_CHECK_OK);
    const NLSemanticContext storage = *a.world;
    calls = 0;
    fail_at = ordinal;
    enabled = true;
    NLCheckStatus status =
        nl_semantic_check_expression(a.world, a.entry_syntax, &a.root, NULL);
    enabled = false;
    assert(status == NL_CHECK_OUT_OF_MEMORY && calls >= ordinal && !a.root);
    assert(nl_copy_state_matches(a.world, &before) &&
           a.world->types == storage.types &&
           a.world->type_count == storage.type_count &&
           a.world->functions == storage.functions &&
           a.world->function_count == storage.function_count &&
           a.world->last_incarnation == storage.last_incarnation &&
           a.world->last_value_fact == storage.last_value_fact);
    nl_copy_state_destroy(&before);
    assert(nl_semantic_check_expression(a.world, a.entry_syntax, &a.root,
                                        NULL) == NL_CHECK_OK);
    (void)auth_proof(&a);
    auth_dispose(&a);
    printf("AUTH OOM ledger rollback/no-clobber/partial-cleanup/retry=PASS\n");
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    setvbuf(stdout, NULL, _IONBF, 0);
    size_t ordinal = auth_census(argv[1]);
    auth_oom(argv[1], ordinal);
    return 0;
}
