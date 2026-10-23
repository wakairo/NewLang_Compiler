/* R769 independent NON-BLIND calibrated tests. Do not alter R765 freeze. */
#include "../../src/copy_body_internal.h"
#include "../support/node_checked.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool inject;
static size_t site, attempts;
void *__wrap_malloc(size_t n)
{
    if (inject && attempts++ == site)
        return NULL;
    return __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t m)
{
    if (inject && attempts++ == site)
        return NULL;
    return __real_calloc(n, m);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (inject && attempts++ == site)
        return NULL;
    return __real_realloc(p, n);
}
static const NLCheckedFragment *caller(const TestNode *n, NLCheckedNodeId *call)
{
    const NLCheckedFragment *f =
        nl_checked_call_body(n->entry, nl_checked_root(n->entry));
    if (!f)
        return NULL;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        NLCopyBodyParameters packet;
        if (nl_checked_copy_body_parameters(f, i, &packet)) {
            *call = i;
            return f;
        }
    }
    return NULL;
}
static bool foreign(const char *path, const char *second_path)
{
    TestNode left = {0}, right = {0};
    CHECK(node_checked_load(path, &left) &&
          node_checked_load(second_path, &right));
    NLCheckedNodeId a = 0, b = 0;
    const NLCheckedFragment *f = caller(&left, &a);
    const NLCheckedFragment *g = caller(&right, &b);
    NLCopyBodyView issued, other;
    NLCopyBodyParameters actual, other_actual;
    CHECK(f && g && nl_checked_copy_body_view(f, a, &issued) &&
          nl_checked_copy_body_view(g, b, &other) &&
          nl_checked_copy_body_parameters(f, a, &actual) &&
          nl_checked_copy_body_parameters(g, b, &other_actual));
    CHECK(left.context != right.context &&
          left.entry_source != right.entry_source && f != g &&
          issued.body != other.body && actual.entry != other_actual.entry &&
          a == b && issued.result.value == other.result.value &&
          actual.field.nominal == other_actual.field.nominal);
    NLSemanticSnapshot sa, sb;
    CHECK(nl_semantic_snapshot(nl_checked_context(f), &sa) &&
          nl_semantic_snapshot(nl_checked_context(g), &sb) &&
          sa.types == sb.types);
    size_t hs = 0;
    for (size_t t = 1; t <= sa.types; ++t) {
        bool ha = nl_semantic_recursive_local_type(nl_checked_context(f), t);
        bool hb = nl_semantic_recursive_local_type(nl_checked_context(g), t);
        CHECK(ha == hb);
        if (ha) {
            NLSemanticTypeView x, y;
            CHECK(nl_semantic_type_view(nl_checked_context(f), t, &x) &&
                  nl_semantic_type_view(nl_checked_context(g), t, &y) &&
                  x.layout_known && y.layout_known && x.size == y.size &&
                  x.alignment == y.alignment && x.size > 0);
            ++hs;
        }
    }
    CHECK(hs == 1);
    CHECK(nl_checked_copy_body_validate(f, a, &issued) == NL_CHECK_OK &&
          nl_checked_copy_body_validate(g, b, &other) == NL_CHECK_OK &&
          nl_checked_copy_body_parameters_validate(f, a, &actual) ==
              NL_CHECK_OK &&
          nl_checked_copy_body_parameters_validate(g, b, &other_actual) ==
              NL_CHECK_OK);
    size_t denied = 0;
#define DENY_BODY(expr)                                                        \
    do {                                                                       \
        NLCopyBodyView changed = issued;                                       \
        expr;                                                                  \
        CHECK(nl_checked_copy_body_validate(f, a, &changed) != NL_CHECK_OK);   \
        ++denied;                                                              \
    } while (0)
    DENY_BODY(changed = other);
    DENY_BODY(changed.body = other.body);
    DENY_BODY(changed.live_exit = other.live_exit);
    DENY_BODY(changed.function++);
    DENY_BODY(changed.root++);
    DENY_BODY(changed.event_count++);
    DENY_BODY(changed.result.value++);
    DENY_BODY(changed.result.type++);
#undef DENY_BODY
#define DENY_PARAM(expr)                                                       \
    do {                                                                       \
        NLCopyBodyParameters changed = actual;                                 \
        expr;                                                                  \
        CHECK(nl_checked_copy_body_parameters_validate(f, a, &changed) !=      \
              NL_CHECK_OK);                                                    \
        ++denied;                                                              \
    } while (0)
    DENY_PARAM(changed = other_actual);
    DENY_PARAM(changed.entry = other_actual.entry);
    DENY_PARAM(changed.handoff = other_actual.handoff);
    DENY_PARAM(changed.loan++);
    DENY_PARAM(changed.arguments[0]++);
    DENY_PARAM(changed.arguments[1]++);
    DENY_PARAM(changed.parameters[0]++);
    DENY_PARAM(changed.parameters[1]++);
    DENY_PARAM(changed.field.nominal++);
    DENY_PARAM(changed.field.parent++);
    DENY_PARAM(changed.field.child++);
    DENY_PARAM(changed.field.index = 1 - actual.field.index);
    DENY_PARAM(changed.field.parent_incarnation++);
    DENY_PARAM(changed.field.child_incarnation++);
    DENY_PARAM(changed.field.parent_fact++);
    DENY_PARAM(changed.field.child_fact++);
    DENY_PARAM(changed.field.parent_post_fact++);
    DENY_PARAM(changed.field.child_post_fact++);
    DENY_PARAM(changed.field.old_value++);
    DENY_PARAM(changed.field.new_value++);
#undef DENY_PARAM
    for (size_t i = 0; i < issued.event_count; ++i) {
        NLCopyBodyEventView event, other_event;
        CHECK(nl_checked_copy_body_event(f, a, i, &event) &&
              nl_checked_copy_body_event(g, b, i, &other_event) &&
              nl_checked_copy_body_event_validate(f, a, i, &event) ==
                  NL_CHECK_OK);
        NLCopyBodyEventView changed = other_event;
        CHECK(nl_checked_copy_body_event_validate(f, a, i, &changed) !=
              NL_CHECK_OK);
        ++denied;
        changed = event;
        changed.before = other_event.before;
        CHECK(nl_checked_copy_body_event_validate(f, a, i, &changed) !=
              NL_CHECK_OK);
        ++denied;
        changed = event;
        changed.after = other_event.after;
        CHECK(nl_checked_copy_body_event_validate(f, a, i, &changed) !=
              NL_CHECK_OK);
        ++denied;
    }
    /* Private-state corruption is diagnostic, not public injectability. */
    NLSemanticContext *current = (NLSemanticContext *)nl_checked_context(f);
    NLSemanticContext *handoff = (NLSemanticContext *)actual.handoff;
    NLTypeId h = 0;
    for (NLTypeId t = 1; t <= sa.types; ++t)
        if (nl_semantic_recursive_local_type(current, t))
            h = t;
    CHECK(h != 0);
#define DENY_PRIVATE(change, restore)                                          \
    do {                                                                       \
        change;                                                                \
        CHECK(nl_checked_copy_body_validate(f, a, NULL) != NL_CHECK_OK);       \
        restore;                                                               \
        CHECK(nl_checked_copy_body_validate(f, a, &issued) == NL_CHECK_OK);    \
        ++denied;                                                              \
    } while (0)
    NLSemanticTypeView original = current->types[h - 1].view;
    DENY_PRIVATE(current->types[h - 1].view.layout_known = false,
                 current->types[h - 1].view = original);
    DENY_PRIVATE(current->types[h - 1].view.size += 8,
                 current->types[h - 1].view = original);
    DENY_PRIVATE(current->types[h - 1].view.alignment *= 2,
                 current->types[h - 1].view = original);
    const NLCheckedNodeView *loan = nl_checked_node_view(f, actual.loan);
    CHECK(loan && loan->loan.scope > 0 &&
          loan->loan.scope <= handoff->scope_count);
    NLSemanticScopeView initial_scope = handoff->scopes[loan->loan.scope - 1];
    CHECK(initial_scope.active);
    DENY_PRIVATE(handoff->scopes[loan->loan.scope - 1].active = false,
                 handoff->scopes[loan->loan.scope - 1] = initial_scope);
    NLSemanticPlaceView initial_child = handoff->places[actual.field.child - 1];
    DENY_PRIVATE(handoff->places[actual.field.child - 1].current_fact++,
                 handoff->places[actual.field.child - 1] = initial_child);
#undef DENY_PRIVATE
    CHECK(nl_checked_copy_body_validate(f, a, &issued) == NL_CHECK_OK &&
          nl_checked_copy_body_parameters_validate(f, a, &actual) ==
              NL_CHECK_OK);
    printf("R769 FOREIGN same-numeric-T=%zu different-world=YES "
           "H-layout-same=YES denied=%zu restoration=PASS\n",
           actual.field.nominal, denied);
    node_checked_destroy(&left);
    node_checked_destroy(&right);
    return true;
}
static bool parse_tree(const char *path, NLSource **src, NLSyntaxTree **tree)
{
    NLParser *parser = NULL;
    CHECK(nl_source_load(path, src) == NL_SOURCE_OK &&
          nl_parser_create(*src, &parser) == NL_PARSE_OK &&
          nl_parser_parse_function_unit(parser, tree, NULL) == NL_PARSE_OK);
    nl_parser_destroy(parser);
    return true;
}
static bool preserve(NLSemanticContext **context)
{
    const char *code = "fn intact()->u8{return u8(39);}";
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    NLParser *parser = NULL;
    CHECK(nl_source_create(code, strlen(code), "r769-preserved", &source) ==
              NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_function_unit(parser, &tree, NULL) == NL_PARSE_OK &&
          nl_semantic_create(context) == NL_CHECK_OK);
    const NLSyntaxTree *input[] = {tree};
    CHECK(nl_semantic_register_function_unit(*context, input, 1, NULL) ==
          NL_CHECK_OK);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}
static bool selected(size_t index, size_t total)
{
    return index < 32 || index == total / 2 ||
           (total >= 3 && index == total - 3) ||
           (total >= 2 && index == total - 2) || index == total - 1;
}
static bool parser_oom(const char *path)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK);
    site = SIZE_MAX;
    attempts = 0;
    inject = true;
    NLParseStatus code = nl_parser_parse_function_unit(parser, &tree, NULL);
    inject = false;
    size_t total = attempts;
    CHECK(code == NL_PARSE_OK && tree && total >= 32);
    nl_syntax_tree_destroy(tree);
    size_t hit = 0;
    for (size_t n = 0; n < total; ++n) {
        if (!selected(n, total))
            continue;
        site = n;
        attempts = 0;
        tree = NULL;
        inject = true;
        code = nl_parser_parse_function_unit(parser, &tree, NULL);
        inject = false;
        CHECK(attempts > n && code == NL_PARSE_OUT_OF_MEMORY && tree == NULL);
        ++hit;
    }
    CHECK(nl_parser_parse_function_unit(parser, &tree, NULL) == NL_PARSE_OK);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    printf("R769 PARSER OOM total-sites=%zu triggered=%zu first32+mid+tail "
           "retry=PASS\n",
           total, hit);
    return true;
}
/* Exact source-owned preexisting identity, not a prelude slot by index. */
typedef struct {
    size_t index;
    const NLFunctionBody *body;
} R772Intact;

static bool r772_preserved_functions(const NLSemanticContext *ctx,
                                    const R772Intact *baseline,
                                    R772Intact *observed)
{
    static const struct {
        const char *name;
        NLCheckedKind kind;
    } prelude[] = {
        {"LifetimeDomain", NL_CHECKED_DOMAIN_CREATE},
        {"ptr_from_ref", NL_CHECKED_PTR_FROM_REF},
        {"finalize_domain", NL_CHECKED_DOMAIN_FINALIZE},
        {"initialize", NL_CHECKED_INITIALIZE},
        {"take", NL_CHECKED_TAKE},
        {"destroy", NL_CHECKED_DESTROY},
        {"replace", NL_CHECKED_REPLACE},
        {"store", NL_CHECKED_STORE},
        {"swap", NL_CHECKED_SWAP},
    };
    const size_t count = sizeof(prelude) / sizeof(prelude[0]);
    bool seen[sizeof(prelude) / sizeof(prelude[0])] = {false};
    size_t intact_count = 0;
    R772Intact intact = {0};
    CHECK(ctx != NULL && ctx->function_count == count + 1);
    for (size_t i = 0; i < ctx->function_count; ++i) {
        const NLFunctionEntry *entry = &ctx->functions[i];
        CHECK(entry->name != NULL);
        if (!strcmp(entry->name, "intact")) {
            CHECK(entry->kind == NL_CHECKED_REGISTERED_CALL &&
                  entry->body != NULL && entry->count == 0);
            intact = (R772Intact){i, entry->body};
            ++intact_count;
            continue;
        }
        size_t j = 0;
        for (; j < count; ++j)
            if (!strcmp(entry->name, prelude[j].name))
                break;
        CHECK(j < count && !seen[j] && entry->kind == prelude[j].kind &&
              entry->body == NULL);
        seen[j] = true;
    }
    CHECK(intact_count == 1);
    for (size_t j = 0; j < count; ++j)
        CHECK(seen[j]);
    if (baseline != NULL)
        CHECK(intact.index == baseline->index && intact.body == baseline->body);
    if (observed != NULL)
        *observed = intact;
    return true;
}
static bool register_oom(const char *path)
{
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(parse_tree(path, &source, &tree));
    const NLSyntaxTree *input[] = {tree};
    NLSemanticContext *probe = NULL, *ctx = NULL;
    CHECK(preserve(&probe));
    site = SIZE_MAX;
    attempts = 0;
    inject = true;
    NLCheckStatus status =
        nl_semantic_register_function_unit(probe, input, 1, NULL);
    inject = false;
    size_t total = attempts;
    CHECK(status == NL_CHECK_OK && total >= 32);
    nl_semantic_destroy(probe);
    CHECK(preserve(&ctx));
    TestState *prior = malloc(sizeof(*prior));
    R772Intact intact = {0};
    CHECK(prior && test_state(ctx, prior) &&
          r772_preserved_functions(ctx, NULL, &intact));
    printf("R772 PRECONDITION nine-prelude=PASS intact-index=%zu "
           "ten-functions=PASS\n",
           intact.index);
    size_t hit = 0;
    for (size_t n = 0; n < total; ++n) {
        if (!selected(n, total))
            continue;
        site = n;
        attempts = 0;
        inject = true;
        status = nl_semantic_register_function_unit(ctx, input, 1, NULL);
        inject = false;
        CHECK(attempts > n && status == NL_CHECK_OUT_OF_MEMORY &&
              test_unchanged(ctx, prior) &&
              r772_preserved_functions(ctx, &intact, NULL));
        printf("R772 REGISTRATION fail-at=%zu status=OOM "
               "original-state-and-intact=PASS\n",
               n);
        ++hit;
    }
    CHECK(nl_semantic_register_function_unit(ctx, input, 1, NULL) ==
          NL_CHECK_OK);
    NLSemanticSnapshot after;
    CHECK(nl_semantic_snapshot(ctx, &after) &&
          after.functions > prior->counts.functions &&
          after.backing_regions == prior->counts.backing_regions &&
          after.domains == prior->counts.domains &&
          after.places == prior->counts.places);
    printf("R769 REGISTRATION OOM total-sites=%zu triggered=%zu "
           "preserved-context=PASS retry=PASS no-allocation-right=PASS\n",
           total, hit);
    free(prior);
    nl_semantic_destroy(ctx);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    return true;
}
static NLCheckStatus validation(size_t mode, const NLCheckedFragment *f,
                                NLCheckedNodeId call,
                                const NLCopyBodyView *body,
                                const NLCopyBodyParameters *params)
{
    if (mode == 0)
        return nl_checked_copy_body_validate(f, call, body);
    if (mode == 1)
        return nl_checked_copy_body_parameters_validate(f, call, params);
    return nl_checked_copy_body_event_validate(f, call, 0, NULL);
}
static bool validation_oom(const char *path)
{
    TestNode checked = {0};
    CHECK(node_checked_load(path, &checked));
    NLCheckedNodeId call = 0;
    const NLCheckedFragment *f = caller(&checked, &call);
    NLCopyBodyView body;
    NLCopyBodyParameters params;
    CHECK(f && nl_checked_copy_body_view(f, call, &body) &&
          nl_checked_copy_body_parameters(f, call, &params) &&
          body.event_count > 0);
    TestState *current = malloc(sizeof(*current));
    TestState *handoff = malloc(sizeof(*handoff));
    CHECK(current && handoff &&
          test_state((NLSemanticContext *)nl_checked_context(f), current) &&
          test_state((NLSemanticContext *)params.handoff, handoff));
    for (size_t mode = 0; mode < 3; ++mode) {
        site = SIZE_MAX;
        attempts = 0;
        inject = true;
        NLCheckStatus status = validation(mode, f, call, &body, &params);
        inject = false;
        size_t total = attempts, hit = 0;
        CHECK(status == NL_CHECK_OK);
        for (size_t n = 0; n < total; ++n) {
            if (!selected(n, total))
                continue;
            site = n;
            attempts = 0;
            inject = true;
            status = validation(mode, f, call, &body, &params);
            inject = false;
            CHECK(attempts > n && status == NL_CHECK_OUT_OF_MEMORY &&
                  test_unchanged((NLSemanticContext *)nl_checked_context(f),
                                 current) &&
                  test_unchanged((NLSemanticContext *)params.handoff, handoff));
            CHECK(validation(mode, f, call, &body, &params) == NL_CHECK_OK);
            printf("R772 VALIDATOR mode=%zu fail-at=%zu status=OOM "
                   "immutable-and-retry=PASS\n",
                   mode, n);
            ++hit;
        }
        printf("R769 VALIDATOR mode=%zu allocation-sites=%zu fail-sites=%zu "
               "immutable=PASS retry=PASS\n",
               mode, total, hit);
    }
    free(current);
    free(handoff);
    node_checked_destroy(&checked);
    return true;
}
int main(int argc, char **argv)
{
    if (argc == 4 && !strcmp(argv[1], "foreign"))
        return foreign(argv[2], argv[3]) ? 0 : 1;
    if (argc != 3)
        return 2;
    bool ok = !strcmp(argv[1], "oom") && parser_oom(argv[2]) &&
              register_oom(argv[2]) && validation_oom(argv[2]);
    return ok ? 0 : 1;
}
