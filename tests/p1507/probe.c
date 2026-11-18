#define main retained_p1463_main
#include "../p1463/probe.c"
#undef main

typedef struct {
    NLCheckedFragment *owner;
    size_t id;
} Operation;
static Operation operations[512];
static size_t operation_count, copies, joins, loans;
static NLPlaceId original_t, original_child;
static NLIncarnationId original_inc;
static NLCopyBodyParameters copy_parameters[2];
static NLCopyBodyEventView copy_events[2];

static bool trace_direct(NLCheckedFragment *f, size_t depth)
{
    CHECK(f && depth < 16);
    for (size_t id = 1; id <= f->count; ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        CHECK(n);
        CHECK(operation_count < 512);
        operations[operation_count++] = (Operation){f, id};
        if (n->kind == NL_CHECKED_LOAN_HEADER) {
            CHECK(n->loan.body_nonescape_proved &&
                  n->loan.normal_result_forwarded);
            ++loans;
        }
        if (n->kind == NL_CHECKED_MATCH) {
            NLSemanticValueView common;
            CHECK(n->field.present && n->normal_arms == 2 &&
                  !n->normal_frame_unchanged && !n->terminates &&
                  n->field.parent == original_t &&
                  nl_semantic_value_view(f->context, n->field.new_value,
                                         &common) &&
                  !common.scalar_known &&
                  common.dependencies == NL_DEPENDENCY_FREE);
            ++joins;
            printf("MATCH depth=%zu id=%zu field=%d captured=%d unchanged=%d "
                   "normal=%zu\n",
                   depth, id, n->field.present, n->captured_frame_closed,
                   n->normal_frame_unchanged, n->normal_arms);
        }
        NLCopyBodyParameters p;
        if (nl_checked_copy_body_parameters(f, id, &p)) {
            NLCopyBodyView v;
            NLCopyBodyEventView e;
            CHECK(nl_checked_copy_body_view(f, id, &v) &&
                  nl_checked_copy_body_event(f, id, 0, &e) &&
                  nl_checked_copy_body_validate(f, id, &v) == NL_CHECK_OK &&
                  nl_checked_copy_body_parameters_validate(f, id, &p) ==
                      NL_CHECK_OK &&
                  nl_checked_copy_body_event_validate(f, id, 0, &e) ==
                      NL_CHECK_OK);
            CHECK(copies < 2 && p.field.index < 2 && v.event_count == 1 &&
                  e.kind == NL_COPY_BODY_CHANGE);
            if (!copies) {
                original_t = p.field.parent;
                original_child = p.field.child;
                original_inc = p.field.parent_incarnation;
            }
            CHECK(p.field.parent == original_t &&
                  p.field.child == original_child &&
                  p.field.parent_incarnation == original_inc);
            NLSemanticPlaceView before, after, parent, sibling;
            NLSemanticValueView old, next, other;
            CHECK(
                nl_semantic_place_view(p.entry, p.field.child, &before) &&
                nl_semantic_place_view(p.handoff, p.field.child, &after) &&
                nl_semantic_place_view(p.handoff, p.field.parent, &parent) &&
                nl_semantic_place_view(p.handoff,
                                       parent.fixed_fields[1 - p.field.index],
                                       &sibling) &&
                nl_semantic_value_view(p.entry, before.current_value, &old) &&
                nl_semantic_value_view(p.handoff, after.current_value, &next) &&
                nl_semantic_value_view(p.handoff, sibling.current_value,
                                       &other) &&
                old.scalar_known &&
                old.scalar_value == (copies          ? 7u
                                     : p.field.index ? 91u
                                                     : 37u) &&
                (next.scalar_known &&
                 next.scalar_value ==
                     (copies ? p.field.index ? 173u : 0u : 7u)) &&
                other.scalar_known &&
                other.scalar_value == (p.field.index ? 37u : 91u) &&
                before.current_fact != after.current_fact);
            copy_parameters[copies] = p;
            copy_events[copies++] = e;
            printf("COPY depth=%zu node=%zu root=%zu child=%zu index=%zu "
                   "entry-regions=%zu entry-domains=%zu\n",
                   depth, id, p.field.parent, p.field.child, p.field.index,
                   p.entry->region_count, p.entry->domain_count);
        }
        const NLCheckedFragment *body = nl_checked_call_body(f, id);
        if (body)
            CHECK(trace_direct((NLCheckedFragment *)body, depth + 1));
        for (size_t a = 0; a < n->item_count && n->kind == NL_CHECKED_MATCH;
             ++a)
            CHECK(trace_direct(
                (NLCheckedFragment *)nl_checked_match_arm(f, id, a),
                depth + 1));
    }
    return true;
}

static bool owned(const char *path, Program *p)
{
    copies = joins = loans = operation_count = 0;
    CHECK(ready(path, p) && actual(p) && trace_direct(p->artifact, 0));
    CHECK(copies == 2 && joins == 2 && loans == 8);
    return true;
}
static bool attacks(const char *path)
{
    Program p = {0}, foreign = {0};
    CHECK(owned(path, &p) && ready(path, &foreign) && actual(&foreign));
    size_t checks = 0;
    for (size_t i = 0; i < operation_count; ++i) {
        Operation op = operations[i];
        NLCheckedNodeView *n = &op.owner->nodes[op.id - 1];
        NLCheckedNodeView saved = *n;
        bool damage = true;
        switch (n->kind) {
        case NL_CHECKED_DESTROY:
        case NL_CHECKED_DOMAIN_FINALIZE:
        case NL_CHECKED_DEALLOCATE:
        case NL_CHECKED_ERASE_SLOT:
        case NL_CHECKED_REF_FROM_PTR:
            n->first_argument = 0;
            break;
        case NL_CHECKED_MATCH:
            n->field.child_post_fact = n->field.child_fact;
            break;
        case NL_CHECKED_LOAN_HEADER:
            n->loan.body_nonescape_proved = false;
            break;
        case NL_CHECKED_REGISTERED_CALL:
            n->function = 0;
            break;
        default:
            damage = false;
        }
        if (damage) {
            bool ok = refused(&p, "direct operation/current/actual tamper");
            *n = saved;
            CHECK(ok);
            ++checks;
        }
        if (n->kind == NL_CHECKED_REGISTERED_CALL && n->result_count == 1) {
            n->results[0].value = 0;
            bool ok = refused(&p, "actual old result substitution");
            *n = saved;
            CHECK(ok);
            ++checks;
        }
        if (n->kind == NL_CHECKED_MATCH) {
            n->field.index = 1 - n->field.index;
            bool ok = refused(&p, "wrong original T field/current sibling");
            *n = saved;
            CHECK(ok);
            n->normal_arms = 1;
            ok = refused(&p, "H2 witness substituted for normal failure edge");
            *n = saved;
            CHECK(ok);
            checks += 2;
        }
    }
    struct NLHOriginOwner *origin = p.artifact->h_origin;
    p.artifact->h_origin = foreign.artifact->h_origin;
    bool rejected = refused(&p, "same numeric IDs, foreign genuine origin");
    p.artifact->h_origin = origin;
    CHECK(rejected);
    for (size_t k = 0; k < 2; ++k) {
        NLCopyBodyParameters wrong = copy_parameters[k];
        wrong.field.parent = wrong.field.child;
        Operation call = {0};
        for (size_t i = 0; i < operation_count; ++i) {
            NLCopyBodyParameters q;
            if (nl_checked_copy_body_parameters(operations[i].owner,
                                                operations[i].id, &q) &&
                q.entry == copy_parameters[k].entry)
                call = operations[i];
        }
        CHECK(call.owner && nl_checked_copy_body_parameters_validate(
                                call.owner, call.id, &wrong) != NL_CHECK_OK);
        const NLSemanticContext *context = call.owner->context;
        call.owner->context = foreign.context;
        rejected = refused(&p, "foreign world for authentic call");
        call.owner->context = (NLSemanticContext *)context;
        CHECK(rejected);
        const NLSource *source = call.owner->source;
        call.owner->source = foreign.source;
        rejected = refused(&p, "foreign source for authentic call");
        call.owner->source = source;
        CHECK(rejected);
    }
    CHECK(emit(&p, NL_NODE_C_OK));
    printf("P1507 %zu operation/loan/join attacks + foreign issuer/world/root "
           "PASS\n",
           checks);
    destroy(&p);
    destroy(&foreign);
    return true;
}
static bool reject_source(const char *path)
{
    Program p = {0};
    CHECK(load(path, &p));
    NLCopyCallAnchor anchor = {0};
    NLSemanticSnapshot before, after;
    CHECK(nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK &&
          nl_semantic_snapshot(p.context, &before));
    NLFunctionUnitDiagnostic d = {0};
    NLCheckStatus s = define(&p, &d);
    printf("P1507 source reject %s status=%d code=%s\n", path, (int)s,
           d.diagnostic.diagnostic.code ? d.diagnostic.diagnostic.code
                                        : "none");
    CHECK((s == NL_CHECK_SEMANTIC_ERROR ||
           s == NL_CHECK_ANALYSIS_PRECISION_LIMIT) &&
          !p.artifact && nl_copy_state_matches(p.context, &anchor) &&
          nl_semantic_snapshot(p.context, &after) &&
          !memcmp(&before, &after, sizeof before));
    nl_copy_state_destroy(&anchor);
    destroy(&p);
    return true;
}
static bool failure(const char *path, unsigned point)
{
    Program p = {0};
    CHECK(ready(path, &p));
    if (point > 2)
        CHECK(actual(&p));
    NLCopyCallAnchor anchor = {0};
    NLSemanticSnapshot before, after;
    CHECK(nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK &&
          nl_semantic_snapshot(p.context, &before));
    hits = text_allocs = 0;
    if (point <= 2) {
        clone_target = p.context;
        fault = point;
        NLCheckStatus s =
            nl_semantic_check_expression(p.context, p.call, &p.artifact, NULL);
        fault = 0;
        clone_target = NULL;
        CHECK(hits == 1 && s == NL_CHECK_OUT_OF_MEMORY && !p.artifact);
    } else {
        if (point == 3)
            fault = 3;
        else {
            copies = joins = loans = operation_count = 0;
            CHECK(trace_direct(p.artifact, 0));
            clone_target = copy_events[point - 4].before;
            CHECK(clone_target);
            fault = 2;
        }
        bool ok = emit(&p, NL_NODE_C_OUT_OF_MEMORY);
        fault = 0;
        clone_target = NULL;
        CHECK(ok && hits == 1);
    }
    CHECK(nl_copy_state_matches(p.context, &anchor) &&
          nl_semantic_snapshot(p.context, &after) &&
          !memcmp(&before, &after, sizeof before));
    if (point <= 2)
        CHECK(actual(&p));
    CHECK(emit(&p, NL_NODE_C_OK));
    printf("P1507 OOM point%u hit once, NULL/length/context rollback + retry "
           "PASS\n",
           point);
    nl_copy_state_destroy(&anchor);
    destroy(&p);
    return true;
}
int main(int argc, char **argv)
{
    Program p = {0};
    if (argc == 3 && !strcmp(argv[1], "attack"))
        return attacks(argv[2]) ? 0 : 1;
    if (argc == 3 && !strcmp(argv[1], "negative"))
        return reject_source(argv[2]) ? 0 : 1;
    if (argc == 4 && !strcmp(argv[1], "oom"))
        return failure(argv[2], (unsigned)strtoul(argv[3], NULL, 10)) ? 0 : 1;
    if (argc != 2 || !owned(argv[1], &p))
        return 1;
    puts("P1507 genuine source-issued MACHINE/origin/Copy validators PASS");
    destroy(&p);
    return 0;
}
