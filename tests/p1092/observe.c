#include "../../src/semantic_internal.h"
#include "../support/raw_storage_check.h"
#include <stdlib.h>

/* P-owned observation of real source execution. Interposition records actual
 * primitive calls; it does not replace a semantic operation or issue proof. */
static bool watch, observed_ok;
static size_t events, selected_index, initial[2], replacement;
static NLPlaceId selected_root, selected_child, sibling;
static NLIncarnationId root_identity, child_identity;
static NLValueFactId root_fact, child_fact, sibling_fact;
static NLSemanticContext *actual_world;
static int fault;
static const NLControlExitView *foreign_edge;
static const NLCheckedFragment *foreign_body;
static const NLCheckedFragment *foreign_parent;
static NLSymbolId ambient_owner, ambient_domain, ambient_allocation,
    ambient_storage;
static NLDomainId ambient_domain_id;
static NLBackingRegionId ambient_region;

NLCheckStatus __real_nl_fixed_change(NLSemanticContext *, NLPlaceId, NLValueId,
                                     bool);
static size_t scalar(const NLSemanticContext *c, NLValueId v)
{
    if (!v || v > c->value_count || !c->values[v - 1].scalar_known) {
        observed_ok = false;
        return 0;
    }
    return c->values[v - 1].scalar_value;
}
NLCheckStatus __wrap_nl_fixed_change(NLSemanticContext *c, NLPlaceId child,
                                     NLValueId incoming, bool discard)
{
    if (!watch)
        return __real_nl_fixed_change(c, child, incoming, discard);
    const NLSemanticPlaceView p = c->places[child - 1];
    if (events++ == 0) {
        actual_world = c;
        selected_root = p.parent_aggregate;
        selected_child = child;
        const NLSemanticPlaceView r = c->places[selected_root - 1];
        sibling = r.fixed_fields[1 - selected_index];
        root_identity = r.incarnation;
        child_identity = p.incarnation;
        root_fact = r.current_fact;
        child_fact = p.current_fact;
        sibling_fact = c->places[sibling - 1].current_fact;
        observed_ok &=
            p.parent_field_index == selected_index &&
            scalar(c, r.current_value ? c->values[r.current_value - 1].fields[0]
                                      : 0) == initial[0] &&
            scalar(c, r.current_value ? c->values[r.current_value - 1].fields[1]
                                      : 0) == initial[1] &&
            scalar(c, incoming) == replacement;
    } else {
        observed_ok &=
            events == 2 && actual_world == c && child == sibling &&
            p.parent_aggregate == selected_root &&
            scalar(c, incoming) == initial[selected_index] &&
            scalar(c, c->places[selected_child - 1].current_value) ==
                replacement &&
            c->places[selected_root - 1].incarnation == root_identity &&
            c->places[selected_child - 1].incarnation == child_identity &&
            c->places[selected_child - 1].current_fact != child_fact &&
            p.current_fact == sibling_fact;
    }
    NLCheckStatus status = __real_nl_fixed_change(c, child, incoming, discard);
    if (status == NL_CHECK_OK && events == 1) {
        observed_ok &= c->places[selected_root - 1].current_fact != root_fact &&
                       c->places[child - 1].current_fact != child_fact &&
                       c->places[sibling - 1].current_fact == sibling_fact &&
                       scalar(c, c->places[sibling - 1].current_value) ==
                           initial[1 - selected_index];
    }
    return status;
}

NLCheckStatus __real_nl_control_source_break_append(
    NLControlExits **, NLControlTarget *, const NLControlState *,
    NLCheckedResult, const NLCheckedFragment *, NLCheckedNodeId,
    const NLCheckedFragment *);
NLCheckStatus __wrap_nl_control_source_break_append(
    NLControlExits **out, NLControlTarget *target, const NLControlState *state,
    NLCheckedResult value, const NLCheckedFragment *owner, NLCheckedNodeId node,
    const NLCheckedFragment *parent)
{
    NLCheckStatus status = __real_nl_control_source_break_append(
        out, target, state, value, owner, node,
        watch && fault == 21 ? foreign_parent : parent);
    if (status != NL_CHECK_OK || !watch || !fault)
        return status;
    NLControlExitView *e = (NLControlExitView *)nl_control_exit_view(*out, 0);
    NLCheckedNodeView *op = NULL;
    for (size_t i = 1; i <= owner->count; ++i)
        if (owner->nodes[i - 1].kind == NL_CHECKED_REPLACE)
            op = &owner->nodes[i - 1];
    if (!op || !actual_world) {
        observed_ok = false;
        return status;
    }
    switch (fault) {
    case 1:
        e->state = foreign_edge->state;
        break;
    case 2:
        e->target = e->source_target = foreign_edge->target;
        break;
    case 3:
        e->source_owner = foreign_body;
        break;
    case 4:
        *e = *foreign_edge;
        break;
    case 5:
        op->field.child = sibling;
        break;
    case 6:
        op->field.old_value = op->field.new_value;
        break;
    case 7:
        op->field.new_value = op->field.old_value;
        break;
    case 8:
        op->kind = NL_CHECKED_STORE;
        break;
    case 9:
        actual_world->places[selected_child - 1].current_fact = child_fact;
        break;
    case 10:
        ++actual_world->places[sibling - 1].current_fact;
        break;
    case 11:
        e->values[0].value = op->field.new_value;
        break;
    case 12:
        actual_world->values[value.value - 1].dependencies =
            NL_DEPENDENCIES_UNKNOWN;
        break;
    case 13:
        actual_world->scopes[actual_world->scope_count - 1].active = true;
        break;
    case 14:
        op->field.parent_fact = op->field.parent_post_fact;
        break;
    case 15:
        op->field.parent_incarnation = 0;
        break;
    case 16:
        op->field.dependency_compatible = false;
        break;
    case 17:
        actual_world
            ->values[actual_world->bindings[ambient_owner - 1].view.value - 1]
            .carrier = NL_CARRIER_ENDED;
        break;
    case 18:
        actual_world->domains[ambient_domain_id - 1].live = false;
        break;
    case 19:
        actual_world->regions[ambient_region - 1].view.ordinary_write = false;
        break;
    case 20:
        actual_world
            ->values[actual_world->bindings[ambient_owner - 1].view.value - 1]
            .dependencies = NL_DEPENDENCIES_UNKNOWN;
        break;
    case 22: {
        NLSemanticContext *post =
            nl_control_state_context((NLControlState *)e->state);
        post->bindings[ambient_owner - 1].view.availability = NL_CONSUMED;
        break;
    }
    case 23:
        if (nl_control_exit_append(out, NL_EXIT_BREAK, target, state, &value,
                                   1) != NL_CHECK_OK)
            observed_ok = false;
        break;
    case 24:
        nl_control_exits_destroy(*out);
        *out = NULL;
        break;
    case 25:
        ((NLCheckedNodeView *)nl_checked_node_view(
             owner, owner->nodes[node - 1].initializer))
            ->symbol = op->field.base;
        break;
    }
    return status;
}

typedef struct {
    NLSource *source, *entry_source;
    NLSyntaxTree *unit, *entry;
    NLSemanticContext *context;
} Input;
static void input_destroy(Input *p)
{
    nl_semantic_destroy(p->context);
    nl_syntax_tree_destroy(p->entry);
    nl_source_destroy(p->entry_source);
    nl_syntax_tree_destroy(p->unit);
    nl_source_destroy(p->source);
}
static bool input_create(const char *path, Input *p)
{
    NLParser *parser = NULL;
    CHECK(nl_source_load(path, &p->source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(p->source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &p->unit, NULL) == NL_PARSE_OK);
    nl_parser_destroy(parser);
    CHECK(nl_semantic_create(&p->context) == NL_CHECK_OK);
    /* Trusted host ambient frame, explicitly distinct from the source-issued
     * Panel/Thermal original and source Break. No ambient fixture is used to
     * issue current/loan authority for the tested source. */
    NLTypeId owner = 0;
    CHECK(nl_semantic_nominal(p->context, "P1092Ambient", false, false,
                              &owner) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_value(p->context, "ambient_owner", owner,
                                 NL_DEPENDENCY_FREE,
                                 &ambient_owner) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_domain(p->context, "ambient_domain", &ambient_domain,
                                  &ambient_domain_id) == NL_CHECK_OK);
    CHECK(raw_allocate(p->context, 8, 8, true, true, "ambient_allocation",
                       "ambient_storage", &ambient_allocation, &ambient_storage,
                       &ambient_region));
    const NLSyntaxTree *units[] = {p->unit};
    CHECK(nl_semantic_register_function_unit(p->context, units, 1, NULL) ==
          NL_CHECK_OK);
    CHECK(nl_source_create("main()", 6, "P1092-entry", &p->entry_source) ==
          NL_SOURCE_OK);
    parser = NULL;
    CHECK(nl_parser_create(p->entry_source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(parser, &p->entry, NULL) ==
          NL_PARSE_OK);
    nl_parser_destroy(parser);
    return true;
}
static const NLCheckedFragment *find_loop(const NLCheckedFragment *f,
                                          const NLCheckedFragment **parent,
                                          NLCheckedNodeId *id)
{
    for (size_t i = 1; i <= f->count; ++i) {
        if (f->nodes[i - 1].kind == NL_CHECKED_LOOP) {
            *parent = f;
            *id = i;
            return nl_checked_loop_body(f, i);
        }
    }
    for (size_t i = 0; i < f->body_count; ++i) {
        const NLCheckedFragment *found = find_loop(f->bodies[i], parent, id);
        if (found)
            return found;
    }
    return NULL;
}

#ifndef P1092_OOM
static bool observe(const char *path)
{
    Input a = {0}, b = {0};
    CHECK(input_create(path, &a) && input_create(path, &b));
    NLCheckedFragment *foreign = NULL;
    CHECK(nl_semantic_check_expression(b.context, b.entry, &foreign, NULL) ==
          NL_CHECK_OK);
    const NLCheckedFragment *parent = NULL;
    NLCheckedNodeId loop = 0;
    foreign_body = find_loop(foreign, &parent, &loop);
    foreign_parent = parent;
    CHECK(foreign_body && nl_checked_loop_break_provenance_valid(parent, loop));
    foreign_edge = nl_control_exit_view(foreign_body->exits, 0);
    for (fault = 0; fault <= 25; ++fault) {
        NLSemanticContext *c = NULL;
        CHECK(nl_sem_clone(a.context, &c) == NL_CHECK_OK);
        TestState before;
        CHECK(test_state(c, &before));
        NLCheckedFragment *checked = NULL;
        NLCheckDiagnostic diagnostic = {0};
        events = 0;
        observed_ok = true;
        watch = true;
        NLCheckStatus status =
            nl_semantic_check_expression(c, a.entry, &checked, &diagnostic);
        watch = false;
        bool ok = observed_ok;
        if (fault) {
            ok &= status != NL_CHECK_OK && checked == NULL &&
                  test_unchanged(c, &before);
        } else {
            const NLCheckedFragment *own_parent = NULL;
            NLCheckedNodeId own_loop = 0;
            const NLCheckedFragment *body =
                checked ? find_loop(checked, &own_parent, &own_loop) : NULL;
            ok &= status == NL_CHECK_OK && events == 2 && body &&
                  body->context != c &&
                  nl_checked_loop_break_provenance_valid(own_parent, own_loop);
        }
        /* Destruction also uses the original private handles when a foreign
         * whole edge was left installed. The independent live B must survive.
         */
        nl_checked_destroy(checked);
        nl_semantic_destroy(c);
        ok &= nl_checked_loop_break_provenance_valid(parent, loop);
        printf("P1092 observe fault=%d status=%d changes=%zu pair/frame/B=%d\n",
               fault, status, events, ok);
        if (!ok) {
            nl_checked_destroy(foreign);
            input_destroy(&a);
            input_destroy(&b);
            return false;
        }
    }
    nl_checked_destroy(foreign);
    input_destroy(&a);
    input_destroy(&b);
    return true;
}
#endif

#ifdef P1092_OOM
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t allocation, fail_at, bytes;
static char kind;
static void *caller;
static bool denied(char k, size_t n, void *pc)
{
    if (!injecting || allocation++ != fail_at)
        return false;
    kind = k;
    bytes = n;
    caller = pc;
    return true;
}
void *__wrap_malloc(size_t n)
{
    return denied('m', n, __builtin_return_address(0)) ? NULL
                                                       : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t z)
{
    return denied('c', n * z, __builtin_return_address(0))
               ? NULL
               : __real_calloc(n, z);
}
void *__wrap_realloc(void *p, size_t n)
{
    return denied('r', n, __builtin_return_address(0)) ? NULL
                                                       : __real_realloc(p, n);
}
static bool oom(const char *path)
{
    observed_ok = true;
    Input input = {0};
    CHECK(input_create(path, &input));
    for (fail_at = 0; fail_at < 20000; ++fail_at) {
        NLSemanticContext *c = NULL;
        CHECK(nl_sem_clone(input.context, &c) == NL_CHECK_OK);
        NLCheckedFragment *prior = NULL;
        CHECK(nl_semantic_check_expression(c, input.entry, &prior, NULL) ==
              NL_CHECK_OK);
        const NLCheckedFragment *parent = NULL;
        NLCheckedNodeId loop = 0;
        CHECK(find_loop(prior, &parent, &loop));
        TestState before;
        CHECK(test_state(c, &before));
        NLCheckedFragment *candidate = NULL;
        NLCheckDiagnostic d = {0};
        allocation = bytes = 0;
        kind = 0;
        caller = NULL;
        injecting = true;
        NLCheckStatus status =
            nl_semantic_check_expression(c, input.entry, &candidate, &d);
        injecting = false;
        if (!kind) {
            bool ok = observed_ok && status == NL_CHECK_OK && candidate &&
                      nl_checked_loop_break_provenance_valid(parent, loop);
            nl_checked_destroy(candidate);
            nl_checked_destroy(prior);
            nl_semantic_destroy(c);
            input_destroy(&input);
            printf("P1092 COMPLETE reached=%zu status=%d ok=%d\n", fail_at,
                   status, ok);
            return ok;
        }
        bool ok = observed_ok && status == NL_CHECK_OUT_OF_MEMORY &&
                  !candidate && d.diagnostic.code &&
                  test_unchanged(c, &before) &&
                  nl_checked_loop_break_provenance_valid(parent, loop);
        printf(
            "P1092 OOM n=%zu kind=%c bytes=%zu pc=%p status=%d rollback=%d\n",
            fail_at, kind, bytes, caller, status, ok);
        nl_checked_destroy(candidate);
        candidate = NULL;
        NLCheckStatus retry =
            nl_semantic_check_expression(c, input.entry, &candidate, NULL);
        ok &= retry == NL_CHECK_OK && candidate &&
              nl_checked_loop_break_provenance_valid(parent, loop);
        nl_checked_destroy(candidate);
        nl_checked_destroy(prior);
        nl_semantic_destroy(c);
        if (!ok) {
            input_destroy(&input);
            fprintf(stderr, "P1092 FIRST RED n=%zu status=%d retry=%d\n",
                    fail_at, status, retry);
            return false;
        }
    }
    input_destroy(&input);
    return false;
}
#endif

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc != 6)
        return 2;
    selected_index = (size_t)strtoul(argv[2], NULL, 10);
    initial[0] = (size_t)strtoul(argv[3], NULL, 10);
    initial[1] = (size_t)strtoul(argv[4], NULL, 10);
    replacement = (size_t)strtoul(argv[5], NULL, 10);
    if (selected_index > 1)
        return 2;
#ifdef P1092_OOM
    return oom(argv[1]) ? 0 : 1;
#else
    return observe(argv[1]) ? 0 : 1;
#endif
}
