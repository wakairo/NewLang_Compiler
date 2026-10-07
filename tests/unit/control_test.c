#include "../../src/control.h"
#include "../../src/semantic_internal.h"
#include "../support/function_body.h"

#include <stdlib.h>
#include <string.h>

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
static size_t failure_at, allocations;
void *__wrap_malloc(size_t n)
{
    if (failure_at != 0 && ++allocations == failure_at)
        return NULL;
    return __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (failure_at != 0 && ++allocations == failure_at)
        return NULL;
    return __real_realloc(p, n);
}

typedef struct {
    NLSemanticContext *context;
    NLControlState *entry;
    NLControlTarget *loop, *inner, *function;
    NLTypeId copy, affine;
    NLValueId initial, owner;
} Fixture;
static bool setup(Fixture *f)
{
    CHECK(nl_semantic_create(&f->context) == NL_CHECK_OK);
    f->copy = nl_semantic_core_type(f->context, NL_TYPE_U8);
    CHECK(nl_semantic_nominal(f->context, "Owner", false, false, &f->affine) ==
          NL_CHECK_OK);
    CHECK(nl_sem_new_value(f->context,
                           (NLSemanticValueView){.type = f->copy,
                                                 .scalar_known = true,
                                                 .scalar_value = 3},
                           &f->initial) == NL_CHECK_OK);
    CHECK(nl_sem_new_value(f->context, (NLSemanticValueView){.type = f->affine},
                           &f->owner) == NL_CHECK_OK);
    CHECK(nl_control_state_create(f->context, &f->entry) == NL_CHECK_OK);
    CHECK(nl_control_target_create(NL_TARGET_LOOP, &f->loop) == NL_CHECK_OK);
    CHECK(nl_control_target_create(NL_TARGET_LOOP, &f->inner) == NL_CHECK_OK);
    CHECK(nl_control_target_create(NL_TARGET_FUNCTION, &f->function) ==
          NL_CHECK_OK);
    return true;
}
static void finish(Fixture *f)
{
    nl_control_state_destroy(f->entry);
    nl_control_target_destroy(f->loop);
    nl_control_target_destroy(f->inner);
    nl_control_target_destroy(f->function);
    nl_semantic_destroy(f->context);
}
static bool edge(NLControlExits **set, Fixture *f, NLControlExitKind kind,
                 NLControlState *state, NLValueId value)
{
    NLCheckedResult result = {
        value == 0 ? 1
                   : nl_control_state_context(state)->values[value - 1].type,
        value};
    CHECK(nl_control_exit_append(set, kind,
                                 kind == NL_EXIT_RETURN ? f->function : f->loop,
                                 state, &result, 1) == NL_CHECK_OK);
    return true;
}
static bool exits(void)
{
    Fixture f = {0};
    CHECK(setup(&f));
    NLControlExits *none = NULL, *ret = NULL, *left = NULL, *right = NULL,
                   *nested = NULL;
    CHECK(nl_control_exits_count(none) == 0);
    CHECK(!nl_control_exits_only_return(none, f.function));
    CHECK(nl_control_function_boundary(none, f.function, 1, 0, 0) ==
          NL_CHECK_OK);
    CHECK(edge(&ret, &f, NL_EXIT_RETURN, f.entry, 0));
    CHECK(edge(&ret, &f, NL_EXIT_RETURN, f.entry, 0));
    CHECK(nl_control_exits_count(ret) == 2 &&
          nl_control_exits_only_return(ret, f.function));
    CHECK(nl_control_function_boundary(ret, f.function, 1, 0, 0) ==
          NL_CHECK_OK);
    CHECK(nl_control_function_boundary(ret, f.function, f.copy, 0, 0) ==
          NL_CHECK_SEMANTIC_ERROR);
    CHECK(edge(&left, &f, NL_EXIT_CONTINUE, f.entry, f.initial));
    CHECK(edge(&left, &f, NL_EXIT_CONTINUE, f.entry, f.initial));
    CHECK(edge(&right, &f, NL_EXIT_BREAK, f.entry, f.initial));
    CHECK(nl_control_exits_union(&nested, ret) == NL_CHECK_OK);
    CHECK(nl_control_exits_union(&nested, left) == NL_CHECK_OK);
    CHECK(nl_control_exits_union(&nested, right) == NL_CHECK_OK);
    CHECK(nl_control_exits_count(nested) == 5);
    CHECK(!nl_control_exits_only_return(nested, f.function));
    CHECK(nl_control_function_boundary(nested, f.function, 1, 0, 0) ==
          NL_CHECK_INTERNAL_ERROR);
    CHECK(nl_control_exit_view(nested, 2)->kind == NL_EXIT_CONTINUE);
    CHECK(nl_control_exit_view(nested, 4)->kind == NL_EXIT_BREAK);
    CHECK(!nl_control_target_same(f.loop, f.inner));
    NLCheckedResult r = {f.copy, f.initial};
    CHECK(nl_control_exit_append(&nested, NL_EXIT_CONTINUE, f.inner, f.entry,
                                 &r, 1) == NL_CHECK_OK);
    CHECK(!nl_control_target_same(nl_control_exit_view(nested, 2)->target,
                                  nl_control_exit_view(nested, 5)->target));
    NLControlOutcome return_flow = {.exits = ret},
                     continue_flow = {.exits = left},
                     break_flow = {.exits = right};
    NLControlOutcome mixed = {0}, outer = {0}, block = {0}, empty = {0},
                     normal = {.normal = true};
    CHECK(nl_control_outcome_join(&return_flow, &continue_flow, &mixed) ==
          NL_CHECK_OK);
    CHECK(!mixed.normal && nl_control_exits_count(mixed.exits) == 4);
    CHECK(nl_control_outcome_join(&break_flow, &continue_flow, &outer) ==
          NL_CHECK_OK);
    CHECK(!outer.normal && nl_control_exits_count(outer.exits) == 3);
    CHECK(nl_control_outcome_then(&mixed, &normal, &block) == NL_CHECK_OK);
    CHECK(!block.normal && nl_control_exits_count(block.exits) == 4);
    nl_control_outcome_destroy(&block);
    CHECK(nl_control_outcome_then(&empty, &return_flow, &block) == NL_CHECK_OK);
    CHECK(!block.normal &&
          nl_control_exits_count(block.exits) == 0); /* no fabricated Return */
    nl_control_outcome_destroy(&block);
    CHECK(nl_control_outcome_join(&normal, &return_flow, &block) ==
          NL_CHECK_OK);
    CHECK(block.normal && nl_control_exits_count(block.exits) == 2);
    nl_control_outcome_destroy(&mixed);
    nl_control_outcome_destroy(&outer);
    nl_control_outcome_destroy(&block);
    /* Wrong target KIND cannot alias a function/loop just because values match.
     */
    CHECK(nl_control_exit_append(&nested, NL_EXIT_RETURN, f.loop, f.entry, &r,
                                 1) == NL_CHECK_INTERNAL_ERROR);
    CHECK(nl_control_exit_append(&nested, NL_EXIT_BREAK, f.function, f.entry,
                                 &r, 1) == NL_CHECK_INTERNAL_ERROR);
    /* Snapshots and targets remain alive after input owners disappear. */
    finish(&f);
    CHECK(nl_control_exit_view(nested, 2)->state != NULL);
    CHECK(nl_control_state_context(
              (NLControlState *)nl_control_exit_view(nested, 2)->state) !=
          NULL);
    nl_control_exits_destroy(ret);
    nl_control_exits_destroy(left);
    nl_control_exits_destroy(right);
    nl_control_exits_destroy(nested);
    return true;
}
static bool header(void)
{
    Fixture f = {0};
    CHECK(setup(&f));
    NLLoopHeader *zero = NULL;
    NLControlExits *zero_edges = NULL;
    CHECK(nl_loop_header_create(f.entry, f.loop, NULL, 0, true, &zero) ==
          NL_CHECK_OK);
    CHECK(nl_control_exit_append(&zero_edges, NL_EXIT_CONTINUE, f.loop, f.entry,
                                 NULL, 0) == NL_CHECK_OK);
    CHECK(nl_loop_header_closure(zero, zero_edges) == NL_CHECK_OK);
    nl_loop_header_destroy(zero);
    nl_control_exits_destroy(zero_edges);
    NLLoopHeader *strict = NULL, *wide = NULL, *unchanged = NULL;
    CHECK(nl_loop_header_create(f.entry, f.loop, &f.initial, 1, false,
                                &strict) == NL_CHECK_OK);
    CHECK(nl_loop_header_create(f.entry, f.loop, &f.initial, 1, true, &wide) ==
          NL_CHECK_OK);
    CHECK(nl_loop_header_create(f.entry, f.loop, &f.owner, 1, true,
                                &unchanged) == NL_CHECK_OK);
    CHECK(nl_loop_header_includes(wide, f.entry, &f.initial, 1) == NL_CHECK_OK);
    NLControlState *a = NULL, *b = NULL;
    CHECK(nl_control_state_fork(f.entry, &a) == NL_CHECK_OK);
    CHECK(nl_control_state_fork(f.entry, &b) == NL_CHECK_OK);
    NLValueId x = 0, y = 0;
    CHECK(nl_sem_new_value(nl_control_state_context(a),
                           (NLSemanticValueView){.type = f.copy,
                                                 .scalar_known = true,
                                                 .scalar_value = 7},
                           &x) == NL_CHECK_OK);
    CHECK(nl_sem_new_value(nl_control_state_context(b),
                           (NLSemanticValueView){.type = f.copy,
                                                 .scalar_known = true,
                                                 .scalar_value = 9},
                           &y) == NL_CHECK_OK);
    CHECK(x == y); /* numeric fork coincidence must not select either payload */
    CHECK(nl_loop_header_includes(strict, a, &x, 1) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(nl_loop_header_includes(wide, a, &x, 1) == NL_CHECK_OK);
    CHECK(nl_loop_header_includes(wide, b, &y, 1) == NL_CHECK_OK);
    CHECK(nl_loop_header_includes(unchanged, a, &f.owner, 1) == NL_CHECK_OK);
    NLControlExits *both = NULL;
    CHECK(edge(&both, &f, NL_EXIT_CONTINUE, a, x));
    CHECK(edge(&both, &f, NL_EXIT_CONTINUE, b, y));
    CHECK(nl_loop_header_closure(wide, both) == NL_CHECK_OK);
    CHECK(nl_loop_header_closure(strict, both) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    /* A valid first edge does not excuse the invalid second successor. */
    NLControlExits *bad = NULL;
    CHECK(edge(&bad, &f, NL_EXIT_CONTINUE, f.entry, f.initial));
    CHECK(edge(&bad, &f, NL_EXIT_CONTINUE, a, x));
    CHECK(nl_loop_header_closure(strict, bad) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(edge(&bad, &f, NL_EXIT_RETURN, f.entry, 0));
    CHECK(edge(&bad, &f, NL_EXIT_BREAK, f.entry, f.owner));
    CHECK(nl_loop_header_closure(wide, bad) == NL_CHECK_OK);
    NLCheckedResult inner = {f.copy, f.initial};
    CHECK(nl_control_exit_append(&bad, NL_EXIT_CONTINUE, f.inner, f.entry,
                                 &inner, 1) == NL_CHECK_OK);
    CHECK(nl_loop_header_closure(wide, bad) == NL_CHECK_INTERNAL_ERROR);
    NLControlState *fork_owner_a = NULL, *fork_owner_b = NULL;
    CHECK(nl_control_state_fork(f.entry, &fork_owner_a) == NL_CHECK_OK);
    CHECK(nl_control_state_fork(f.entry, &fork_owner_b) == NL_CHECK_OK);
    NLValueId owner_a = 0, owner_b = 0;
    CHECK(nl_sem_new_value(nl_control_state_context(fork_owner_a),
                           (NLSemanticValueView){.type = f.affine},
                           &owner_a) == NL_CHECK_OK);
    CHECK(nl_sem_new_value(nl_control_state_context(fork_owner_b),
                           (NLSemanticValueView){.type = f.affine},
                           &owner_b) == NL_CHECK_OK);
    CHECK(owner_a == owner_b);
    NLLoopHeader *fork_identity = NULL;
    CHECK(nl_loop_header_create(fork_owner_a, f.loop, &owner_a, 1, true,
                                &fork_identity) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(fork_identity == NULL);
    nl_control_state_destroy(fork_owner_a);
    nl_control_state_destroy(fork_owner_b);
    NLValueId transformed = 0;
    nl_sem_end_value(nl_control_state_context(a), f.owner);
    CHECK(nl_sem_new_value(nl_control_state_context(a),
                           (NLSemanticValueView){.type = f.affine},
                           &transformed) == NL_CHECK_OK);
    CHECK(nl_loop_header_includes(unchanged, a, &transformed, 1) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    NLValueId duplicate[] = {f.owner, f.owner};
    NLLoopHeader *invalid = NULL;
    CHECK(nl_loop_header_create(f.entry, f.loop, duplicate, 2, true,
                                &invalid) == NL_CHECK_SEMANTIC_ERROR);
    CHECK(invalid == NULL);
    nl_control_exits_destroy(both);
    nl_control_exits_destroy(bad);
    nl_loop_header_destroy(strict);
    nl_loop_header_destroy(wide);
    nl_loop_header_destroy(unchanged);
    nl_control_state_destroy(a);
    nl_control_state_destroy(b);
    finish(&f);
    return true;
}
static bool origins(void)
{
    Fixture f = {0};
    CHECK(setup(&f));
    NLSymbolId symbol = 0;
    CHECK(nl_semantic_seed_value(f.context, "captured", f.affine,
                                 NL_DEPENDENCY_FREE, &symbol) == NL_CHECK_OK);
    NLPlaceId place = 0;
    NLValueId root = 0;
    CHECK(nl_sem_new_value(f.context, (NLSemanticValueView){.type = f.copy},
                           &root) == NL_CHECK_OK);
    CHECK(nl_sem_new_place(f.context, f.copy, 0, true, root, &place) ==
          NL_CHECK_OK);
    nl_control_state_destroy(f.entry);
    f.entry = NULL;
    CHECK(nl_control_state_create(f.context, &f.entry) == NL_CHECK_OK);
    NLLoopHeader *exact = NULL, *wide = NULL;
    CHECK(nl_loop_header_create(f.entry, f.loop, &f.initial, 1, false,
                                &exact) == NL_CHECK_OK);
    CHECK(nl_loop_header_create(f.entry, f.loop, &f.initial, 1, true, &wide) ==
          NL_CHECK_OK);
    NLControlState *a = NULL, *b = NULL, *alien = NULL;
    CHECK(nl_control_state_fork(f.entry, &a) == NL_CHECK_OK);
    CHECK(nl_control_state_fork(f.entry, &b) == NL_CHECK_OK);
    CHECK(nl_control_state_create(f.context, &alien) == NL_CHECK_OK);
    CHECK(nl_loop_header_includes(wide, alien, &f.initial, 1) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    /* Use ordinary internal semantic constructors; no private state writes. */
    NLValueId av = 0, bv = 0;
    NLValueFactId af = 0, bf = 0;
    CHECK(nl_sem_new_value(nl_control_state_context(a),
                           (NLSemanticValueView){.type = f.copy,
                                                 .scalar_known = true,
                                                 .scalar_value = 7},
                           &av) == NL_CHECK_OK);
    CHECK(nl_sem_new_value(nl_control_state_context(b),
                           (NLSemanticValueView){.type = f.copy,
                                                 .scalar_known = true,
                                                 .scalar_value = 9},
                           &bv) == NL_CHECK_OK);
    CHECK(nl_sem_fresh_fact(nl_control_state_context(a), &af) == NL_CHECK_OK);
    CHECK(nl_sem_fresh_fact(nl_control_state_context(b), &bf) == NL_CHECK_OK);
    CHECK(av == bv && af == bf);
    /* Changing outer Copy facts uses the real store checker. Stable external
     * reference blockers remain exact, while only current Copy contents widen.
     */
    NLScopeId scope = 0;
    NLTypeId ref = 0;
    CHECK(nl_semantic_scope(f.context, 0, true, &scope) == NL_CHECK_OK);
    CHECK(nl_semantic_compound_type(f.context, NL_TYPE_REF, f.copy,
                                    NL_ACCESS_WRITE, false,
                                    &ref) == NL_CHECK_OK);
    NLSemanticPlaceView pv;
    CHECK(nl_semantic_place_view(f.context, place, &pv));
    NLSymbolId write = 0, value = 0;
    CHECK(nl_semantic_seed_reference(
              f.context, "write", ref,
              (NLReferenceFacts){place, pv.incarnation, scope,
                                 NL_PROVENANCE_VALID, true, true, 0},
              &write) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_scalar(f.context, "seven",
                                  (NLScalarValue){f.copy, true, 7},
                                  &value) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_scalar(f.context, "nine",
                                  (NLScalarValue){f.copy, true, 9},
                                  &value) == NL_CHECK_OK);
    /* A two-alternative blocker cannot become a dependency-free/selected
     * alternative merely because fresh numeric tables resemble a prefix. */
    NLValueId other_value = 0, may_ref = 0;
    NLPlaceId other_place = 0;
    CHECK(nl_sem_new_value(f.context, (NLSemanticValueView){.type = f.copy},
                           &other_value) == NL_CHECK_OK);
    CHECK(nl_sem_new_place(f.context, f.copy, 0, true, other_value,
                           &other_place) == NL_CHECK_OK);
    NLSemanticPlaceView other;
    CHECK(nl_semantic_place_view(f.context, other_place, &other));
    NLSemanticValueView may = {
        .type = ref,
        .reference_count = 2,
        .references = {
            {place, pv.incarnation, scope, NL_PROVENANCE_VALID, true, true, 0},
            {other_place, other.incarnation, scope, NL_PROVENANCE_VALID, true,
             true, 0}}};
    CHECK(nl_sem_new_value(f.context, may, &may_ref) == NL_CHECK_OK);
    NLControlState *blockers = NULL, *dropped = NULL;
    CHECK(nl_control_state_create(f.context, &blockers) == NL_CHECK_OK);
    CHECK(nl_control_state_fork(blockers, &dropped) == NL_CHECK_OK);
    NLLoopHeader *blocked_header = NULL;
    CHECK(nl_loop_header_create(blockers, f.loop, &f.initial, 1, true,
                                &blocked_header) == NL_CHECK_OK);
    may.reference_count = 1;
    NLValueId selected = 0;
    nl_sem_end_value(nl_control_state_context(dropped), may_ref);
    CHECK(nl_sem_new_value(nl_control_state_context(dropped), may, &selected) ==
          NL_CHECK_OK);
    CHECK(nl_loop_header_includes(blocked_header, dropped, &f.initial, 1) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    nl_loop_header_destroy(blocked_header);
    nl_control_state_destroy(blockers);
    nl_control_state_destroy(dropped);
    NLControlState *ram = NULL, *seven = NULL, *nine = NULL;
    CHECK(nl_control_state_create(f.context, &ram) == NL_CHECK_OK);
    CHECK(nl_control_state_fork(ram, &seven) == NL_CHECK_OK);
    CHECK(nl_control_state_fork(ram, &nine) == NL_CHECK_OK);
    NLLoopHeader *concrete = NULL, *abstract = NULL;
    CHECK(nl_loop_header_create(ram, f.loop, &f.initial, 1, false, &concrete) ==
          NL_CHECK_OK);
    CHECK(nl_loop_header_create(ram, f.loop, &f.initial, 1, true, &abstract) ==
          NL_CHECK_OK);
    TestChecked store = {0};
    CHECK(test_run(nl_control_state_context(seven), "store(write,seven)",
                   TEST_SOURCE, NL_CHECK_OK, NULL, &store));
    test_checked_destroy(&store);
    CHECK(test_run(nl_control_state_context(nine), "store(write,nine)",
                   TEST_SOURCE, NL_CHECK_OK, NULL, &store));
    test_checked_destroy(&store);
    NLSemanticPlaceView ps, pn;
    CHECK(nl_semantic_place_view(nl_control_state_context(seven), place, &ps));
    CHECK(nl_semantic_place_view(nl_control_state_context(nine), place, &pn));
    CHECK(ps.current_fact == pn.current_fact &&
          ps.current_value == pn.current_value);
    CHECK(nl_loop_header_includes(concrete, seven, &f.initial, 1) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(nl_loop_header_includes(abstract, seven, &f.initial, 1) ==
          NL_CHECK_OK);
    CHECK(nl_loop_header_includes(abstract, nine, &f.initial, 1) ==
          NL_CHECK_OK);
    NLLoopHeader *coincidence = NULL;
    CHECK(nl_loop_header_create(seven, f.loop, &f.initial, 1, false,
                                &coincidence) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(coincidence == NULL);
    nl_loop_header_destroy(concrete);
    nl_loop_header_destroy(abstract);
    nl_control_state_destroy(ram);
    nl_control_state_destroy(seven);
    nl_control_state_destroy(nine);
    CHECK(nl_loop_header_includes(exact, a, &f.initial, 1) ==
          NL_CHECK_OK); /* mere unused fresh IDs are harmless */
    TestChecked used = {0};
    CHECK(test_run(nl_control_state_context(a), "captured", TEST_SOURCE,
                   NL_CHECK_OK, NULL, &used));
    test_checked_destroy(&used);
    CHECK(nl_loop_header_includes(wide, a, &f.initial, 1) ==
          NL_CHECK_SEMANTIC_ERROR);
    NLValueId unknown = 0, hidden = 0;
    CHECK(nl_sem_new_value(
              nl_control_state_context(b),
              (NLSemanticValueView){.type = f.copy,
                                    .dependencies = NL_DEPENDENCIES_UNKNOWN},
              &unknown) == NL_CHECK_OK);
    CHECK(nl_loop_header_includes(wide, b, &unknown, 1) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(nl_sem_new_value(
              nl_control_state_context(b),
              (NLSemanticValueView){.type = f.copy,
                                    .dependencies = NL_HIDDEN_DEPENDENCIES},
              &hidden) == NL_CHECK_OK);
    CHECK(nl_loop_header_includes(wide, b, &hidden, 1) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    nl_loop_header_destroy(exact);
    nl_loop_header_destroy(wide);
    nl_control_state_destroy(a);
    nl_control_state_destroy(b);
    nl_control_state_destroy(alien);
    finish(&f);
    return true;
}
static bool failure(void)
{
    Fixture f = {0};
    CHECK(setup(&f));
    TestState before;
    CHECK(test_state(f.context, &before));
    NLControlExits *set = NULL;
    CHECK(edge(&set, &f, NL_EXIT_CONTINUE, f.entry, f.initial));
    bool success = false;
    for (size_t n = 1; n < 4000; ++n) {
        NLControlExits *out = NULL;
        allocations = 0;
        failure_at = n;
        NLCheckStatus status = nl_control_exits_union(&out, set);
        failure_at = 0;
        if (status == NL_CHECK_OK) {
            success = true;
            nl_control_exits_destroy(out);
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && out == NULL);
        CHECK(test_unchanged(f.context, &before));
    }
    CHECK(success);
    success = false;
    for (size_t n = 1; n < 4000; ++n) {
        NLLoopHeader *h = NULL;
        allocations = 0;
        failure_at = n;
        NLCheckStatus status =
            nl_loop_header_create(f.entry, f.loop, &f.initial, 1, true, &h);
        failure_at = 0;
        if (status == NL_CHECK_OK) {
            success = true;
            nl_loop_header_destroy(h);
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && h == NULL);
        CHECK(test_unchanged(f.context, &before));
    }
    CHECK(success);
    NLControlOutcome input = {.normal = true, .exits = set};
    success = false;
    for (size_t n = 1; n < 4000; ++n) {
        NLControlOutcome joined = {0};
        allocations = 0;
        failure_at = n;
        NLCheckStatus status = nl_control_outcome_join(&input, &input, &joined);
        failure_at = 0;
        if (status == NL_CHECK_OK) {
            success = true;
            CHECK(joined.normal && nl_control_exits_count(joined.exits) == 2);
            nl_control_outcome_destroy(&joined);
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && !joined.normal &&
              joined.exits == NULL);
        CHECK(nl_control_exits_count(set) == 1 &&
              test_unchanged(f.context, &before));
    }
    CHECK(success);
    /* Growth preserves existing edge set on EVERY allocation failure. */
    success = false;
    for (size_t n = 1; n < 4000; ++n) {
        size_t count = nl_control_exits_count(set);
        allocations = 0;
        failure_at = n;
        NLCheckedResult v = {f.copy, f.initial};
        NLCheckStatus status = nl_control_exit_append(&set, NL_EXIT_CONTINUE,
                                                      f.loop, f.entry, &v, 1);
        failure_at = 0;
        if (status == NL_CHECK_OK) {
            success = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY &&
              nl_control_exits_count(set) == count);
        CHECK(test_unchanged(f.context, &before));
    }
    CHECK(success);
    while (nl_control_exits_count(set) < NL_CONTROL_MAX_EXITS)
        CHECK(edge(&set, &f, NL_EXIT_CONTINUE, f.entry, f.initial));
    NLValueId over[NL_CONTROL_MAX_SLOTS + 1] = {0};
    NLLoopHeader *too_large = NULL;
    CHECK(nl_loop_header_create(f.entry, f.loop, over, NL_CONTROL_MAX_SLOTS + 1,
                                true, &too_large) == NL_CHECK_RESOURCE_LIMIT);
    CHECK(too_large == NULL);
    NLCheckedResult v = {f.copy, f.initial};
    CHECK(nl_control_exit_append(&set, NL_EXIT_CONTINUE, f.loop, f.entry, &v,
                                 1) == NL_CHECK_RESOURCE_LIMIT);
    CHECK(nl_control_exits_count(set) == NL_CONTROL_MAX_EXITS);
    CHECK(nl_control_exits_union(&set, set) == NL_CHECK_RESOURCE_LIMIT);
    nl_control_exits_destroy(set);
    finish(&f);
    return true;
}
static bool integration(void)
{
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    NLTypeId boolean = nl_semantic_core_type(c, NL_TYPE_BOOL),
             u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    const NLFunctionParameter params[] = {{"cond", boolean}, {"x", u8}};
    CHECK(register_body(c, "both", params, 2, u8,
                        "{if(cond){return x;}else{{return x;}}}", NL_CHECK_OK,
                        NULL));
    CHECK(register_body(
        c, "nested", params, 2, u8,
        "{if(cond){return x;}else{if(cond){return x;}else{return x;}}}",
        NL_CHECK_OK, NULL));
    NLSymbolId symbol = 0;
    CHECK(nl_semantic_seed_value(c, "flag", boolean, NL_DEPENDENCY_FREE,
                                 &symbol) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_scalar(c, "byte_value",
                                  (NLScalarValue){u8, true, 42},
                                  &symbol) == NL_CHECK_OK);
    const char *calls[] = {"both(flag,byte_value)", "nested(flag,byte_value)"};
    for (size_t i = 0; i < 2; ++i) {
        TestChecked run = {0};
        CHECK(test_run(c, calls[i], TEST_SOURCE, NL_CHECK_OK, NULL, &run));
        const NLCheckedFragment *body =
            nl_checked_call_body(run.artifact, nl_checked_root(run.artifact));
        CHECK(body != NULL);
        const NLControlExits *e = nl_checked_control_exits(body);
        CHECK(nl_control_exits_count(e) == i + 2);
        for (size_t j = 0; j < nl_control_exits_count(e); ++j) {
            const NLControlExitView *v = nl_control_exit_view(e, j);
            CHECK(v->kind == NL_EXIT_RETURN && v->values[0].type == u8);
            NLSemanticValueView value;
            CHECK(nl_semantic_value_view(
                nl_control_state_context((NLControlState *)v->state),
                v->values[0].value, &value));
            CHECK(value.scalar_known && value.scalar_value == 42);
        }
        test_checked_destroy(&run);
    }
    NLTypeId sum = 0;
    const NLSumVariant variants[] = {{"A", 0}, {"B", 0}};
    CHECK(nl_semantic_register_sum(c, "Tag", variants, 2, &sum) == NL_CHECK_OK);
    const NLFunctionParameter match_params[] = {{"tag", sum}, {"x", u8}};
    CHECK(register_body(c, "matched", match_params, 2, u8,
                        "{match tag {A=>{return x;},B=>{return x;},}}",
                        NL_CHECK_OK, NULL));
    TestChecked matched = {0};
    CHECK(test_run(c, "matched(Tag::A,byte_value)", TEST_SOURCE, NL_CHECK_OK,
                   NULL, &matched));
    const NLCheckedFragment *body = nl_checked_call_body(
        matched.artifact, nl_checked_root(matched.artifact));
    CHECK(nl_control_exits_count(nl_checked_control_exits(body)) >= 2);
    test_checked_destroy(&matched);
    nl_semantic_destroy(c);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    bool ok = strcmp(argv[1], "exits") == 0         ? exits()
              : strcmp(argv[1], "header") == 0      ? header()
              : strcmp(argv[1], "origins") == 0     ? origins()
              : strcmp(argv[1], "failure") == 0     ? failure()
              : strcmp(argv[1], "integration") == 0 ? integration()
                                                    : false;
    return ok ? 0 : 1;
}
