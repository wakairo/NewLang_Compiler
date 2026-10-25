#include "../../src/copy_body_internal.h"
#include "../support/node_checked.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define REQUIRE(x)                                                             \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "P896 line%d: %s\n", __LINE__, #x);                \
            abort();                                                           \
        }                                                                      \
    } while (0)
static size_t records, verified, rejected, faults, oom;
static bool mutate, expect_bad;
static uintptr_t world[2];
static void refuses(const NLDefinitionCallCapture *p, NLDefinitionCallView v)
{
    NLDefinitionCallResult r;
    REQUIRE(nl_definition_call_validate(p, &v, &r) ==
            NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    ++faults;
}
static void observe(const NLDefinitionCallCapture *p,
                    const NLDefinitionCallView *v, NLCheckStatus status,
                    void *unused)
{
    (void)unused;
    if (status == NL_CHECK_OUT_OF_MEMORY) {
        ++oom;
        return;
    }
    REQUIRE(p && v && status == NL_CHECK_OK);
    ++records;
    NLDefinitionCallResult r;
    NLCheckStatus s = nl_definition_call_validate(p, v, &r);
    printf("P896 arm%zu call%zu PRE=%d RETURN_OLD=%d CLOSED=%d status%d "
           "old%llu incoming%llu sibling%llu replacements%zu failure_line%zu\n",
           v->arm, v->call, r.pre_verified, r.return_old_verified,
           r.loan_closed, s, (unsigned long long)r.old_value,
           (unsigned long long)r.incoming, (unsigned long long)r.sibling,
           r.replacements, r.failure_line);
    REQUIRE(r.pre_verified && r.old_known && r.sibling_known);
    if (expect_bad) {
        REQUIRE(s == NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
                !r.return_old_verified);
        ++rejected;
        return;
    }
    REQUIRE(s == NL_CHECK_OK && r.return_old_verified && r.loan_closed &&
            r.replacements == 1);
    world[v->arm] = (uintptr_t)v->world;
    ++verified;
    if (!mutate)
        return;
#define BAD(change)                                                            \
    do {                                                                       \
        NLDefinitionCallView bad = *v;                                         \
        change;                                                                \
        refuses(p, bad);                                                       \
    } while (0)
    BAD(bad.function = v->caller_function);
    BAD(bad.body = v->pre->functions[v->caller_function - 1].body);
    BAD(bad.caller = v->callee);
    BAD(bad.callee = v->caller);
    BAD(bad.world = v->pre);
    BAD(bad.pre = v->post);
    BAD(bad.post = v->pre);
    BAD(bad.arm = 1 - v->arm);
    BAD(bad.arguments[0] = v->arguments[1]);
    BAD(bad.formals[0] = v->formals[1]);
    BAD(bad.field.parent = v->other);
    BAD(bad.field.child = v->sibling);
    BAD(bad.field.index = 1 - v->field.index);
    BAD(++bad.field.parent_incarnation);
    BAD(++bad.field.child_incarnation);
    BAD(++bad.field.parent_fact);
    BAD(++bad.field.child_fact);
    BAD(bad.sibling = v->field.child);
    BAD(bad.other = v->field.parent);
    BAD(bad.scope = 0);
    BAD(bad.loan = 0);
    BAD(bad.call = 0);
    BAD(bad.ordinal = 1);
    BAD(bad.pending_count = 0);
    BAD(bad.pending_count = 2);
#undef BAD
    /* Real owned graph mutations: positive verification must derive from
     * source/graph, not just compare candidate bytes with an issued summary. */
    NLCheckedFragment *caller = (NLCheckedFragment *)v->caller;
    NLCheckedFragment *body = (NLCheckedFragment *)v->callee;
    NLCheckedNodeView saved = caller->nodes[v->call - 1];
    caller->nodes[v->call - 1].function = v->caller_function;
    refuses(p, *v);
    caller->nodes[v->call - 1] = saved;
    NLCheckedNodeView loan_saved = caller->nodes[v->loan - 1];
    caller->nodes[v->loan - 1].loan.scope = 0;
    refuses(p, *v);
    caller->nodes[v->loan - 1] = loan_saved;
    NLSemanticContext *c = (NLSemanticContext *)v->world;
    c->scopes[v->scope - 1].active = true;
    refuses(p, *v);
    c->scopes[v->scope - 1].active = false;
    NLFunctionBody *owner = body->body_owner;
    body->body_owner = v->pre->functions[v->caller_function - 1].body;
    refuses(p, *v);
    body->body_owner = owner;
    NLControlTarget *target = body->function_target;
    body->function_target = v->parent->function_target;
    refuses(p, *v);
    body->function_target = target;
    for (size_t i = 0; i < body->count; ++i) {
        NLCheckedNodeView old = body->nodes[i];
        if (old.kind == NL_CHECKED_REPLACE) {
            ++body->nodes[i].field.child_post_fact;
            refuses(p, *v);
            body->nodes[i] = old;
            body->nodes[i].field.child = v->sibling;
            refuses(p, *v);
            body->nodes[i] = old;
        }
        if (old.kind == NL_CHECKED_RETURN) {
            body->nodes[i].initializer = 0;
            refuses(p, *v);
            body->nodes[i] = old;
            NLCheckedNodeView value = body->nodes[old.initializer - 1];
            body->nodes[old.initializer - 1].symbol = v->formals[1];
            refuses(p, *v);
            body->nodes[old.initializer - 1] = value;
        }
    }
    const NLCheckedNodeView *loan = nl_checked_node_view(caller, v->loan);
    NLCheckedNodeView block = caller->nodes[loan->initializer - 1];
    caller->nodes[loan->initializer - 1].tail = 0;
    refuses(p, *v);
    caller->nodes[loan->initializer - 1] = block;
    caller->nodes[loan->initializer - 1].item_count = 1;
    caller->nodes[loan->initializer - 1].first_item = v->call;
    refuses(p, *v);
    caller->nodes[loan->initializer - 1] = block;
    caller->nodes[v->call - 1].first_argument = v->arguments[1];
    refuses(p, *v);
    caller->nodes[v->call - 1] = saved;
    NLValueFactId sibling_fact = c->places[v->sibling - 1].current_fact;
    ++c->places[v->sibling - 1].current_fact;
    refuses(p, *v);
    c->places[v->sibling - 1].current_fact = sibling_fact;
    NLValueFactId other_fact = c->places[v->other - 1].current_fact;
    ++c->places[v->other - 1].current_fact;
    refuses(p, *v);
    c->places[v->other - 1].current_fact = other_fact;
    NLSemanticContext *pre = (NLSemanticContext *)v->pre;
    pre->scopes[v->scope - 1].active = false;
    refuses(p, *v);
    pre->scopes[v->scope - 1].active = true;
    NLIncarnationId incarnation = pre->places[v->field.child - 1].incarnation;
    ++pre->places[v->field.child - 1].incarnation;
    refuses(p, *v);
    pre->places[v->field.child - 1].incarnation = incarnation;
    if (v->arm == 1 && v->parent->arm_count == 2) {
        NLCheckedFragment *foreign = v->parent->arms[0].artifact;
        NLCheckedFragment *saved_body = caller->bodies[0];
        REQUIRE(foreign != caller && foreign->context != caller->context &&
                foreign->body_count == 1 && foreign->body_calls[0] == v->call);
        caller->bodies[0] = foreign->bodies[0];
        refuses(p, *v);
        caller->bodies[0] = saved_body;
        /* Same numerical RETURN value IDs, different arm-owned state. */
        NLControlExitView *edge =
            (NLControlExitView *)nl_control_exit_view(body->exits, 0);
        const NLControlExitView *other_edge =
            nl_control_exit_view(foreign->bodies[0]->exits, 0);
        REQUIRE(edge && other_edge &&
                edge->values[0].value == other_edge->values[0].value);
        const NLControlState *state = edge->state;
        edge->state = other_edge->state;
        refuses(p, *v);
        edge->state = state;
    }
    REQUIRE(nl_definition_call_validate(p, v, &r) == NL_CHECK_OK);
}
#ifdef P896_OOM
static bool capture_active, inject;
static size_t allocation_index, failure_index;
void *__real_calloc(size_t, size_t);
void *__wrap_calloc(size_t n, size_t size)
{
    if (capture_active && allocation_index++ == failure_index && inject) {
        inject = false;
        return NULL;
    }
    return __real_calloc(n, size);
}
NLDefinitionCallCapture *
__real_nl_definition_call_begin(const NLCheckedFragment *, NLCheckedNodeId,
                                const NLCheckedNodeId[2], size_t,
                                const NLSemanticContext *);
NLDefinitionCallCapture *
__wrap_nl_definition_call_begin(const NLCheckedFragment *f, NLCheckedNodeId id,
                                const NLCheckedNodeId a[2], size_t function,
                                const NLSemanticContext *c)
{
    capture_active = true;
    NLDefinitionCallCapture *p =
        __real_nl_definition_call_begin(f, id, a, function, c);
    capture_active = false;
    return p;
}
#endif
static bool run(const char *path, bool negative, bool no_call)
{
    NLSource *s = NULL;
    NLParser *p = NULL;
    NLSyntaxTree *tree = NULL;
    NLSemanticContext *c = NULL;
    CHECK(nl_source_load(path, &s) == NL_SOURCE_OK &&
          nl_parser_create(s, &p) == NL_PARSE_OK &&
          nl_parser_parse_function_unit(p, &tree, NULL) == NL_PARSE_OK &&
          nl_semantic_create(&c) == NL_CHECK_OK);
    TestState *before = calloc(1, sizeof(*before));
    CHECK(before && test_state(c, before));
    records = verified = rejected = oom = 0;
    world[0] = world[1] = 0;
    expect_bad = negative;
    nl_definition_call_observer(observe, NULL);
    const NLSyntaxTree *units[] = {tree};
    NLFunctionUnitDiagnostic d = {0};
    NLCheckStatus status = nl_semantic_register_function_unit(c, units, 1, &d);
    nl_definition_call_observer(NULL, NULL);
    if (no_call) {
        NLDefinitionCallView fake = {0};
        NLDefinitionCallResult invalid;
        CHECK(status == NL_CHECK_OK && !records && !oom &&
              nl_definition_call_validate(NULL, &fake, &invalid) ==
                  NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    } else {
        CHECK(status == NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
              !strcmp(d.diagnostic.diagnostic.code, "P6-JOIN-PRECISION"));
        /* Failed registration is transactional: initial public unit is empty.
         */
        CHECK(test_unchanged(c, before));
        if (!oom) {
            CHECK(records == 2 && (negative ? rejected == 2 : verified == 2));
            if (!negative)
                CHECK(world[0] && world[1] && world[0] != world[1]);
        }
#ifdef P896_OOM
        if (oom) {
            size_t first_oom = oom;
            records = verified = rejected = oom = 0;
            nl_definition_call_observer(observe, NULL);
            status = nl_semantic_register_function_unit(c, units, 1, &d);
            nl_definition_call_observer(NULL, NULL);
            CHECK(status == NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
                  !strcmp(d.diagnostic.diagnostic.code, "P6-JOIN-PRECISION") &&
                  records == 2 && verified == 2 && !oom &&
                  test_unchanged(c, before));
            oom = first_oom;
            puts("P896 same-context retry after capture OOM: source CHECK3 "
                 "unchanged, two verified diagnostic records");
        }
#endif
    }
    free(before);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(p);
    nl_source_destroy(s);
    nl_semantic_destroy(c);
    return true;
}
int main(int argc, char **argv)
{
    if (argc < 3)
        return 2;
    mutate = !strcmp(argv[1], "negative");
#ifdef P896_OOM
    if (!strcmp(argv[1], "oom")) {
        for (size_t at = 0; at < 2; ++at) {
            allocation_index = 0;
            failure_index = at;
            inject = true;
            REQUIRE(run(argv[2], false, false));
            REQUIRE(oom == 1 && !inject);
            REQUIRE(run(argv[2], false, false) && records == 2 &&
                    verified == 2 && !oom);
        }
        puts("P896 capture calloc and PRE-clone calloc reached, no public "
             "artifact; clean retry OK");
        return 0;
    }
#endif
    for (int i = 2; i < argc; ++i) {
        bool bad = strstr(argv[i], "negative_return_") != NULL;
        bool none = strstr(argv[i], "no_call") != NULL;
        if (!run(argv[i], bad, none))
            return 1;
    }
    REQUIRE(!mutate || faults >= 60);
    printf("P896 private mutations rejected%zu; never public proof/admission\n",
           faults);
    return 0;
}
