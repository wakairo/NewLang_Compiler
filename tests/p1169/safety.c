#define main p1169_cli_main
#include "main.c"
#undef main
#include <assert.h>
#include <dirent.h>

typedef struct {
    NLSource *source, *entry;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *syntax, *entry_syntax;
    NLSemanticContext *world;
    NLCheckedFragment *root;
    NLCheckedFragment *retained_owner;
    NLCheckedNodeId retained_loop;
} Input;
static void parse(Input *a, const char *path)
{
    assert(nl_source_load(path, &a->source) == NL_SOURCE_OK);
    assert(nl_parser_create(a->source, &a->parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(a->parser, &a->syntax, NULL) ==
           NL_PARSE_OK);
    assert(nl_source_create("main()", 6, "C238 actual source invocation",
                            &a->entry) == NL_SOURCE_OK);
    assert(nl_parser_create(a->entry, &a->entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(
               a->entry_parser, &a->entry_syntax, NULL) == NL_PARSE_OK);
    assert(nl_semantic_create(&a->world) == NL_CHECK_OK);
}
static NLCheckStatus registration(Input *a)
{
    const NLSyntaxTree *units[] = {a->syntax};
    return nl_semantic_register_function_unit(a->world, units, 1, NULL);
}
static NLCheckStatus invocation(Input *a)
{
    return nl_semantic_check_expression(a->world, a->entry_syntax, &a->root,
                                        NULL);
}
static void dispose(Input *a)
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
static NLCheckedFragment *owner(Input *a)
{
    return (NLCheckedFragment *)nl_checked_call_body(a->root,
                                                     nl_checked_root(a->root));
}
static NLCheckedNodeId loop_id(Input *a)
{
    NLCheckedFragment *f = owner(a);
    assert(f);
    NLCheckedNodeId loop = 0;
    for (size_t i = 0; i < f->count; ++i)
        if (f->nodes[i].kind == NL_CHECKED_LOOP) {
            assert(!loop);
            loop = i + 1;
        }
    assert(loop);
    return loop;
}
static bool valid(Input *a, NLLinearBreakView *out)
{
    if (!a->retained_owner) {
        a->retained_owner = owner(a);
        if (!a->retained_owner)
            return false;
        a->retained_loop = loop_id(a);
    }
    return nl_linear_break_validate(a->retained_owner, a->retained_loop, out) ==
               NL_CHECK_OK &&
           out->changing_current && out->scalar_loop;
}

#ifdef P1169_OOM
static bool enabled;
static size_t calls, fail_at;
static void *sites[8192];
static char kinds[8192];
static bool inject(char kind, void *pc)
{
    if (!enabled)
        return false;
    if (calls < sizeof sites / sizeof sites[0]) {
        sites[calls] = pc;
        kinds[calls] = kind;
    }
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
static void site_list(const char *stage, size_t total)
{
    assert(total <= sizeof sites / sizeof sites[0]);
    for (size_t i = 0; i < total; ++i)
        printf("C238 SITE stage=%s ordinal=%zu kind=%c pc=%p\n", stage, i + 1,
               kinds[i], sites[i]);
}
static size_t census(const char *path, bool actual)
{
    Input a = {0};
    parse(&a, path);
    if (actual)
        assert(registration(&a) == NL_CHECK_OK);
    calls = 0;
    fail_at = SIZE_MAX;
    enabled = true;
    NLCheckStatus status = actual ? invocation(&a) : registration(&a);
    enabled = false;
    assert(status == NL_CHECK_OK);
    const size_t total = calls;
    site_list(actual ? "actual" : "registration", total);
    dispose(&a);
    return total;
}
static void sentinel(const char *path)
{
    FILE *f = fopen(path, "wb");
    assert(f && fputs("C238 original named output\n", f) >= 0 &&
           fclose(f) == 0);
}
static void named(const char *path, bool census_only)
{
    char directory[] = "/tmp/p1169-named-XXXXXX", target[128];
    assert(mkdtemp(directory));
    assert(snprintf(target, sizeof target, "%s/out.c", directory) > 0);
    char *args[] = {"newlangc", "--emit-c17-to", target, (char *)path};
    sentinel(target);
    calls = 0;
    fail_at = SIZE_MAX;
    enabled = true;
    int status = p1169_cli_main(4, args);
    enabled = false;
    assert(status == 0);
    size_t total = calls;
    site_list("named", total);
    printf("C238 CENSUS named=%zu\n", total);
    if (!census_only)
        for (size_t point = 1; point <= total; ++point) {
            sentinel(target);
            struct stat before, after;
            assert(stat(target, &before) == 0);
            calls = 0;
            fail_at = point;
            enabled = true;
            status = p1169_cli_main(4, args);
            enabled = false;
            assert(status != 0 && calls >= point);
            assert(stat(target, &after) == 0 && before.st_ino == after.st_ino &&
                   before.st_mtim.tv_sec == after.st_mtim.tv_sec &&
                   before.st_mtim.tv_nsec == after.st_mtim.tv_nsec);
            FILE *f = fopen(target, "rb");
            char buffer[64] = {0};
            assert(f && fread(buffer, 1, sizeof buffer, f) ==
                            strlen("C238 original named output\n"));
            assert(fclose(f) == 0 &&
                   !strcmp(buffer, "C238 original named output\n"));
            DIR *d = opendir(directory);
            assert(d);
            for (struct dirent *e = readdir(d); e; e = readdir(d))
                assert(!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..") ||
                       !strcmp(e->d_name, "out.c"));
            assert(closedir(d) == 0);
            assert(p1169_cli_main(4, args) == 0);
        }
    assert(unlink(target) == 0 && rmdir(directory) == 0);
    printf("C238 named no-clobber/temporary/retry=%s sites=%zu\n",
           census_only ? "CENSUS_ONLY" : "PASS", total);
}
static void oom(const char *path, bool actual, size_t total)
{
    for (size_t point = 1; point <= total; ++point) {
        Input a = {0};
        parse(&a, path);
        if (actual)
            assert(registration(&a) == NL_CHECK_OK);
        NLCopyCallAnchor before = {0};
        assert(nl_copy_state_anchor(a.world, &before) == NL_CHECK_OK);
        const NLSemanticContext storage = *a.world;
        calls = 0;
        fail_at = point;
        enabled = true;
        NLCheckStatus status = actual ? invocation(&a) : registration(&a);
        enabled = false;
        if (status != NL_CHECK_OUT_OF_MEMORY) {
            fprintf(stderr,
                    "C238 OOM stage=%s point=%zu/%zu status=%d calls=%zu\n",
                    actual ? "actual" : "registration", point, total, status,
                    calls);
        }
        assert(status == NL_CHECK_OUT_OF_MEMORY && calls >= point && !a.root);
        assert(nl_copy_state_matches(a.world, &before));
        assert(a.world->types == storage.types &&
               a.world->type_count == storage.type_count);
        assert(a.world->functions == storage.functions &&
               a.world->function_count == storage.function_count);
        assert(a.world->last_incarnation == storage.last_incarnation &&
               a.world->last_value_fact == storage.last_value_fact);
        nl_copy_state_destroy(&before);
        if (actual)
            assert(invocation(&a) == NL_CHECK_OK);
        else {
            assert(registration(&a) == NL_CHECK_OK);
            assert(invocation(&a) == NL_CHECK_OK);
        }
        NLLinearBreakView view;
        assert(valid(&a, &view));
        dispose(&a);
    }
    printf("C238 OOM stage=%s injected=%zu "
           "rollback/no-clobber/cleanup/retry=PASS\n",
           actual ? "actual" : "registration", total);
}
int main(int argc, char **argv)
{
    assert(argc == 2 || argc == 3);
    const size_t reg = census(argv[1], false), actual = census(argv[1], true);
    printf("C238 CENSUS registration=%zu actual=%zu total=%zu\n", reg, actual,
           reg + actual);
    if (argc == 3) {
        named(argv[1], true);
        return 0;
    }
    oom(argv[1], false, reg);
    oom(argv[1], true, actual);
    named(argv[1], false);
    return 0;
}
#else
int main(int argc, char **argv)
{
    assert(argc == 2);
    Input a = {0};
    parse(&a, argv[1]);
    assert(registration(&a) == NL_CHECK_OK);
    assert(invocation(&a) == NL_CHECK_OK);
    NLLinearBreakView v = {0};
    assert(valid(&a, &v));
    NLCheckedFragment *f = owner(&a);
    NLCheckedFragment *g =
        (NLCheckedFragment *)nl_checked_loop_body(f, loop_id(&a));
    const NLCheckedNodeView *block = nl_checked_node_view(g, g->root);
    NLCheckedFragment *c =
        (NLCheckedFragment *)nl_checked_if_arm(g, block->tail, 0);
    NLCheckedFragment *b =
        (NLCheckedFragment *)nl_checked_if_arm(g, block->tail, 1);
    assert(c && b && v.iteration == c);
    const NLSemanticContext *h = g->context;
    const NLSemanticContext *post = nl_control_state_context(
        (NLControlState *)nl_control_exit_view(c->exits, 0)->state);
    const NLSemanticContext *exit = nl_control_state_context(
        (NLControlState *)nl_control_exit_view(b->exits, 0)->state);
    const NLPlaceId sibling = h->places[v.root - 1].fixed_fields[1 - v.index];
    assert(!h->values[h->places[v.child - 1].current_value - 1].scalar_known);
    assert(post->places[v.root - 1].current_fact > h->last_value_fact);
    assert(post->places[v.child - 1].current_fact > h->last_value_fact);
    assert(post->places[v.root - 1].current_fact !=
           post->places[v.child - 1].current_fact);
    assert(post->places[v.root - 1].incarnation ==
           h->places[v.root - 1].incarnation);
    assert(post->places[v.child - 1].incarnation ==
           h->places[v.child - 1].incarnation);
    assert(post->places[sibling - 1].current_fact ==
           h->places[sibling - 1].current_fact);
    assert(exit->places[v.root - 1].current_fact ==
           h->places[v.root - 1].current_fact);
    assert(exit->places[v.child - 1].current_fact ==
           h->places[v.child - 1].current_fact);
    printf("C238 public-owned=YES private-current=0 index=%zu phase=%zu "
           "root=%zu child=%zu\n",
           v.index, v.phase_symbol, v.root, v.child);
    printf("C238 entry-function=%zu loop=%zu incoming=%zu sibling=%zu "
           "ghost-fresh=YES break-write=NO\n",
           nl_checked_node_view(a.root, nl_checked_root(a.root))->function,
           loop_id(&a), v.incoming,
           h->values[h->places[sibling - 1].current_value - 1].scalar_value);
    for (size_t i = 0; i < c->count; ++i) {
        const NLCheckedNodeView *n = &c->nodes[i];
        if (n->kind == NL_CHECKED_BINDING && n->symbol)
            printf("C238 LOCAL name=%s symbol=%zu\n",
                   c->context->bindings[n->symbol - 1].name, n->symbol);
    }
    /* Finite challenged-view matrix. Every mutation is restored except the
     * final foreign A/B swap used to test canonical destruction. */
    size_t trials = 0;
#define BAD(expr, change)                                                      \
    do {                                                                       \
        change;                                                                \
        assert(!valid(&a, &v));                                                \
        expr;                                                                  \
        assert(valid(&a, &v));                                                 \
        ++trials;                                                              \
    } while (0)
    bool inductive = f->nodes[loop_id(&a) - 1].header_inductive;
    BAD(f->nodes[loop_id(&a) - 1].header_inductive = inductive,
        f->nodes[loop_id(&a) - 1].header_inductive = false);
    size_t count = g->arm_count;
    BAD(g->arm_count = count, g->arm_count = 0);
    BAD(g->arm_count = count, g->arm_count = SIZE_MAX);
    count = a.root->body_count;
    BAD(a.root->body_count = count, a.root->body_count = 0);
    BAD(a.root->body_count = count, a.root->body_count = SIZE_MAX);
    NLSemanticContext *world = (NLSemanticContext *)c->context;
    NLValueFactId fact = world->places[v.child - 1].current_fact;
    BAD(world->places[v.child - 1].current_fact = fact,
        world->places[v.child - 1].current_fact =
            h->places[v.child - 1].current_fact);
    NLIncarnationId incarnation = world->places[v.root - 1].incarnation;
    BAD(world->places[v.root - 1].incarnation = incarnation,
        world->places[v.root - 1].incarnation++);
    fact = world->places[sibling - 1].current_fact;
    BAD(world->places[sibling - 1].current_fact = fact,
        world->places[sibling - 1].current_fact++);
    NLValueId sibling_value = world->places[sibling - 1].current_value;
    NLValueId aggregate_owner =
        world->values[sibling_value - 1].aggregate_owner;
    BAD(world->values[sibling_value - 1].aggregate_owner = aggregate_owner,
        world->values[sibling_value - 1].aggregate_owner =
            h->places[v.root - 1].current_value);
    NLCheckedNodeView node = c->nodes[v.continue_node - 1];
    BAD(c->nodes[v.continue_node - 1] = node,
        c->nodes[v.continue_node - 1].initializer = 0);
    const NLSemanticContext *saved = c->context;
    BAD(c->context = saved, c->context = b->context);
    const NLSource *saved_source = f->source;
    BAD(f->source = saved_source, f->source = a.entry);
    NLCheckedNodeView *operation = NULL;
    for (size_t i = 0; i < c->count; ++i)
        if (c->nodes[i].kind == NL_CHECKED_REPLACE)
            operation = &c->nodes[i];
    assert(operation);
    node = *operation;
    BAD(*operation = node, operation->field.index = 1 - v.index);
    BAD(*operation = node,
        operation->field.old_value = operation->field.new_value);
    BAD(*operation = node, operation->results[0].value = 0);
    NLSemanticContext *header = (NLSemanticContext *)g->context;
    NLValueId child_value = header->places[v.child - 1].current_value;
    NLSemanticValueView package = header->values[child_value - 1];
    BAD(header->values[child_value - 1] = package,
        header->values[child_value - 1].dependencies = NL_DEPENDENCIES_UNKNOWN);
    NLSemanticContext *caller = (NLSemanticContext *)f->context;
    NLFunctionEntry *functions = caller->functions;
    BAD(caller->functions = functions, caller->functions = NULL);
    size_t function =
        nl_checked_node_view(a.root, nl_checked_root(a.root))->function;
    bool effects = functions[function - 1].caller_effects;
    BAD(functions[function - 1].caller_effects = effects,
        functions[function - 1].caller_effects = !effects);
    Input other = {0};
    parse(&other, argv[1]);
    assert(registration(&other) == NL_CHECK_OK &&
           invocation(&other) == NL_CHECK_OK);
    NLLinearBreakView ov;
    assert(valid(&other, &ov));
    NLCheckedFragment *foreign = (NLCheckedFragment *)nl_checked_loop_body(
        owner(&other), loop_id(&other));
    NLControlExitView *edge =
        (NLControlExitView *)nl_control_exit_view(g->exits, 0);
    NLControlExitView saved_edge = *edge;
    BAD(*edge = saved_edge,
        edge->target = nl_control_exit_view(foreign->exits, 0)->target);
    BAD(*edge = saved_edge,
        edge->state = nl_control_exit_view(foreign->exits, 0)->state);
    g->arms[0] = foreign->arms[0];
    assert(!valid(&a, &v));
    dispose(&a); /* A's original child must die, foreign B must survive. */
    assert(valid(&other, &ov));
    dispose(&other);
    printf("C238 tamper=%zu foreign-A-destroy/B-survive=PASS\n", trials);
    for (size_t t = 0; t < 9; ++t) {
        a = (Input){0};
        other = (Input){0};
        parse(&a, argv[1]);
        parse(&other, argv[1]);
        assert(registration(&a) == NL_CHECK_OK &&
               invocation(&a) == NL_CHECK_OK);
        assert(registration(&other) == NL_CHECK_OK &&
               invocation(&other) == NL_CHECK_OK);
        assert(valid(&a, &v) && valid(&other, &ov));
        NLCheckedFragment *ga =
            (NLCheckedFragment *)nl_checked_loop_body(owner(&a), loop_id(&a));
        NLCheckedFragment *gb = (NLCheckedFragment *)nl_checked_loop_body(
            owner(&other), loop_id(&other));
        NLCheckedFragment *fa = t < 3 ? ga : owner(&a);
        NLCheckedFragment *fb = t < 3 ? gb : owner(&other);
        if (t == 0 || t == 3)
            fa->arm_count = 0;
        if (t == 1 || t == 4)
            fa->arm_count = SIZE_MAX;
        if (t == 2 || t == 5)
            fa->arms = fb->arms;
        if (t == 6)
            a.root->body_count = 0;
        if (t == 7)
            a.root->body_count = SIZE_MAX;
        if (t == 8)
            a.root->bodies = other.root->bodies;
        assert(!valid(&a, &v) && valid(&other, &ov));
        dispose(&a);
        assert(valid(&other, &ov));
        dispose(&other);
    }
    puts(
        "C238 unrestored display-count/foreign-table canonical-destroy=9 PASS");
    return 0;
}
#endif
