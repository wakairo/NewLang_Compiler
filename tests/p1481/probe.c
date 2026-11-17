#define main p1463_retained_main
#define __wrap_nl_sem_clone retained_clone_wrapper
#include "../p1463/probe.c"
#undef main
#undef __wrap_nl_sem_clone

static bool join_fault;
static size_t join_hits;
NLCheckStatus __wrap_nl_sem_clone(const NLSemanticContext *c,
                                  NLSemanticContext **out)
{
    if (join_fault && !join_hits && c->region_count == 1 &&
        !c->regions[0].view.live && c->domain_count == 1 &&
        !c->domains[0].live) {
        bool closed = true;
        for (size_t i = 0; i < c->scope_count; ++i)
            closed = closed && !c->scopes[i].active;
        if (closed) {
            ++join_hits;
            clone_target = c;
            fault = 2;
            NLCheckStatus s = retained_clone_wrapper(c, out);
            fault = 0;
            clone_target = NULL;
            printf("closed-H ancestor clone OOM: regions=%zu domains=%zu "
                   "status=%d hits=%zu\n",
                   c->region_count, c->domain_count, (int)s, hits);
            return s;
        }
    }
    return retained_clone_wrapper(c, out);
}

typedef struct {
    size_t joins, calls, loans;
    NLPlaceId root, child;
    NLIncarnationId incarnation;
    const NLCheckedFragment *owners[2];
    NLCheckedNodeId call_ids[2];
    NLCopyBodyParameters parameters[2];
    NLCopyBodyEventView events[2];
} CurrentTrace;

static bool current_walk(NLCheckedFragment *f, CurrentTrace *t, size_t index,
                         unsigned sample, size_t depth)
{
    CHECK(f && depth < 24);
    const NLSemanticContext *c = f->context;
    for (size_t i = 1; i <= f->count; ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        CHECK(n);
        if (n->kind == NL_CHECKED_LOAN_HEADER) {
            CHECK(n->loan.body_nonescape_proved && n->loan.scope &&
                  n->loan.scope <= c->scope_count &&
                  !c->scopes[n->loan.scope - 1].active);
            ++t->loans;
        }
        if (n->kind == NL_CHECKED_MATCH && n->field.present) {
            const NLCheckedField d = n->field;
            NLSemanticValueView value;
            CHECK(n->normal_arms == 2 && n->item_count == 2 && !n->terminates &&
                  !n->normal_frame_unchanged && n->type == 1 &&
                  d.index == index && d.parent == t->root &&
                  d.child == t->child &&
                  d.parent_incarnation == t->incarnation &&
                  d.parent_fact != d.parent_post_fact &&
                  d.child_fact != d.child_post_fact &&
                  nl_semantic_value_view(c, d.new_value, &value) &&
                  !value.scalar_known &&
                  value.dependencies == NL_DEPENDENCY_FREE);
            for (size_t a = 0; a < 2; ++a) {
                const NLCheckedFragment *arm = nl_checked_match_arm(f, i, a);
                const NLCheckedNodeView *r =
                    nl_checked_node_view(arm, arm->root);
                CHECK(r && !r->terminates &&
                      !nl_control_exits_count(arm->exits));
                CHECK(r->field.present == (a == 1));
            }
            ++t->joins;
        }
        NLCopyBodyParameters p;
        if (n->kind == NL_CHECKED_REGISTERED_CALL &&
            nl_checked_copy_body_parameters(f, i, &p)) {
            NLCopyBodyView v;
            NLCopyBodyEventView e;
            CHECK(t->calls < 2 && nl_checked_copy_body_view(f, i, &v) &&
                  nl_checked_copy_body_event(f, i, 0, &e) &&
                  nl_checked_copy_body_validate(f, i, &v) == NL_CHECK_OK &&
                  nl_checked_copy_body_parameters_validate(f, i, &p) ==
                      NL_CHECK_OK &&
                  nl_checked_copy_body_event_validate(f, i, 0, &e) ==
                      NL_CHECK_OK &&
                  e.kind == NL_COPY_BODY_CHANGE && v.event_count == 1 &&
                  p.field.index == index);
            NLSemanticPlaceView a, b, root, sibling;
            NLSemanticValueView old, next, other;
            CHECK(nl_semantic_place_view(p.entry, p.field.child, &a) &&
                  nl_semantic_place_view(p.handoff, p.field.child, &b) &&
                  nl_semantic_value_view(p.entry, a.current_value, &old) &&
                  nl_semantic_value_view(p.handoff, b.current_value, &next) &&
                  nl_semantic_place_view(p.handoff, p.field.parent, &root) &&
                  nl_semantic_place_view(
                      p.handoff, root.fixed_fields[1 - index], &sibling) &&
                  nl_semantic_value_view(p.handoff, sibling.current_value,
                                         &other) &&
                  old.scalar_known && other.scalar_known &&
                  old.scalar_value == (t->calls ? 0 : 87) &&
                  (t->calls ? !next.scalar_known
                            : (next.scalar_known && !next.scalar_value)) &&
                  other.scalar_value == 13);
            if (!t->calls) {
                t->root = p.field.parent;
                t->child = p.field.child;
                t->incarnation = p.field.parent_incarnation;
            } else {
                CHECK(t->root == p.field.parent && t->child == p.field.child &&
                      t->incarnation == p.field.parent_incarnation &&
                      t->owners[0] != f);
            }
            t->owners[t->calls] = f;
            t->call_ids[t->calls] = i;
            t->parameters[t->calls] = p;
            t->events[t->calls++] = e;
        }
        const NLCheckedFragment *body = nl_checked_call_body(f, i);
        if (body)
            CHECK(current_walk((NLCheckedFragment *)body, t, index, sample,
                               depth + 1));
        for (size_t a = 0; a < 2; ++a) {
            const NLCheckedFragment *arm = nl_checked_match_arm(f, i, a);
            if (arm)
                CHECK(current_walk((NLCheckedFragment *)arm, t, index, sample,
                                   depth + 1));
        }
    }
    return true;
}
static bool owned1481(const char *path, size_t index, unsigned sample)
{
    Program p = {0};
    CurrentTrace t = {0};
    CHECK(ready(path, &p) && actual(&p) &&
          current_walk(p.artifact, &t, index, sample, 0) && t.joins == 2 &&
          t.calls == 2 && t.loans == 11);
    printf(
        "P1481 MACHINE two joins/calls; closed loans=%zu root=%zu child=%zu\n",
        t.loans, (size_t)t.root, (size_t)t.child);
    destroy(&p);
    return true;
}

static bool path1481(char *out, size_t cap, const char *dir, const char *name)
{
    int n = snprintf(out, cap, "%s/%s", dir, name);
    return n > 0 && (size_t)n < cap;
}

static bool rejected1481(const char *bad, const char *control)
{
    Program p = {0}, valid = {0};
    NLCopyCallAnchor anchor = {0};
    NLSemanticSnapshot before, after;
    CHECK(load(bad, &p) && load(control, &valid) &&
          nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK &&
          nl_semantic_snapshot(p.context, &before));
    NLFunctionUnitDiagnostic d = {0};
    NLCheckStatus status = define(&p, &d);
    printf("negative %s status=%d code=%s\n", bad, (int)status,
           d.diagnostic.diagnostic.code ? d.diagnostic.diagnostic.code
                                        : "none");
    CHECK((status == NL_CHECK_SEMANTIC_ERROR ||
           status == NL_CHECK_ANALYSIS_PRECISION_LIMIT) &&
          !p.artifact && nl_copy_state_matches(p.context, &anchor) &&
          nl_semantic_snapshot(p.context, &after) &&
          !memcmp(&before, &after, sizeof(before)));
    const NLSyntaxTree *units[] = {valid.unit};
    CHECK(nl_semantic_register_function_unit(p.context, units, 1, &d) ==
              NL_CHECK_OK &&
          actual(&p) && emit(&p, NL_NODE_C_OK));
    nl_copy_state_destroy(&anchor);
    destroy(&p);
    destroy(&valid);
    return true;
}

static bool tamper1481(const char *path)
{
    Program p = {0}, other = {0};
    CHECK(ready(path, &p) && actual(&p) && ready(path, &other) &&
          actual(&other));
    NLCheckedFragment *body = (NLCheckedFragment *)nl_checked_call_body(
        p.artifact, nl_checked_root(p.artifact));
    NLCheckedFragment *foreign_body = (NLCheckedFragment *)nl_checked_call_body(
        other.artifact, nl_checked_root(other.artifact));
    CHECK(body && foreign_body);
    NLCheckedNodeView *join = NULL;
    size_t match = 0;
    for (size_t i = 1; i <= body->count; ++i)
        if (body->nodes[i - 1].kind == NL_CHECKED_MATCH &&
            body->nodes[i - 1].field.present) {
            CHECK(!join);
            join = &body->nodes[i - 1];
            match = i;
        }
    CHECK(join);
    NLCheckedNodeView original = *join;
    join->field.index = 1 - join->field.index;
    CHECK(refused(&p, "join wrong field"));
    *join = original;
    join->field.child_post_fact = join->field.child_fact;
    CHECK(refused(&p, "join stale current"));
    *join = original;
    join->normal_frame_unchanged = true;
    CHECK(refused(&p, "join false unchanged frame"));
    *join = original;
    NLSemanticValueView value =
        body->context->values[join->field.new_value - 1];
    body->context->values[join->field.new_value - 1].scalar_known = true;
    body->context->values[join->field.new_value - 1].scalar_value = 181;
    CHECK(refused(&p, "join forged Known"));
    body->context->values[join->field.new_value - 1] = value;
    NLCheckedArm *edge = NULL;
    size_t count = 0;
    for (size_t i = 0; i < body->arm_count; ++i)
        if (body->arms[i].match == match && ++count == 2)
            edge = &body->arms[i];
    CHECK(edge && count == 2);
    NLCheckedFragment *saved = edge->artifact;
    edge->artifact =
        (NLCheckedFragment *)nl_checked_match_arm(foreign_body, match, 1);
    CHECK(edge->artifact && refused(&p, "foreign genuine Some arm"));
    edge->artifact = saved;
    CurrentTrace t = {0}, foreign = {0};
    CHECK(current_walk(p.artifact, &t, 0, 181, 0) &&
          current_walk(other.artifact, &foreign, 0, 181, 0) && t.calls == 2 &&
          foreign.calls == 2);
    NLCheckedFragment *owner = (NLCheckedFragment *)t.owners[1];
    NLCheckedNodeView *last =
        (NLCheckedNodeView *)nl_checked_node_view(owner, t.call_ids[1]);
    NLCheckedNodeView *loan =
        (NLCheckedNodeView *)nl_checked_node_view(owner, t.parameters[1].loan);
    CHECK(last && loan && owner->context->functions && last->function &&
          last->function <= owner->context->function_count);
#define DAMAGE1481(target, replacement, label)                                 \
    do {                                                                       \
        unsigned char bytes[sizeof(target)];                                   \
        memcpy(bytes, &(target), sizeof(target));                              \
        (target) = (replacement);                                              \
        bool invalid = refused(&p, label);                                     \
        memcpy(&(target), bytes, sizeof(target));                              \
        CHECK(invalid && nl_checked_proof_class(p.artifact) ==                 \
                             NL_CHECKED_MACHINE_PROVED);                       \
    } while (0)
    DAMAGE1481(last->function, 0, "H2 wrong actual function");
    DAMAGE1481(last->first_argument, 0, "H2 fragment-local argument");
    DAMAGE1481(last->results[0].value, 0, "H2 missing actual old result");
    DAMAGE1481(loan->loan.body_nonescape_proved, false, "H2 open loan");
    DAMAGE1481(owner->context, foreign.owners[1]->context, "H2 foreign world");
    DAMAGE1481(owner->source, foreign.owners[1]->source, "H2 foreign source");
    DAMAGE1481(p.artifact->h_origin, other.artifact->h_origin,
               "H2 foreign issuer");
    NLFunctionEntry *fn = &owner->context->functions[last->function - 1];
    DAMAGE1481(fn->parameters, NULL, "H2 missing authentic signature");
#undef DAMAGE1481
    NLCopyBodyParameters wrong = t.parameters[1];
    wrong.field.parent = wrong.field.child;
    CHECK(nl_checked_copy_body_parameters_validate(owner, t.call_ids[1],
                                                   &wrong) ==
              NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
          nl_checked_copy_body_parameters_validate(owner, t.call_ids[1],
                                                   &foreign.parameters[1]) ==
              NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
          nl_checked_copy_body_event_validate(owner, t.call_ids[1], 0,
                                              &foreign.events[1]) ==
              NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(nl_checked_proof_class(p.artifact) == NL_CHECKED_MACHINE_PROVED &&
          emit(&p, NL_NODE_C_OK) && emit(&other, NL_NODE_C_OK));
    destroy(&p);
    destroy(&other);
    return true;
}

static bool replay_oom1481(const char *path)
{
    Program p = {0};
    CurrentTrace t = {0};
    NLCopyCallAnchor anchor = {0};
    NLSemanticSnapshot before, after;
    CHECK(ready(path, &p) && actual(&p) &&
          current_walk(p.artifact, &t, 0, 181, 0) && t.calls == 2 &&
          nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK &&
          nl_semantic_snapshot(p.context, &before));
    clone_target = t.events[1].before;
    CHECK(clone_target);
    hits = 0;
    fault = 2;
    bool ok = emit(&p, NL_NODE_C_OUT_OF_MEMORY);
    fault = 0;
    clone_target = NULL;
    printf("actual H2 Copy replay clone OOM hit=%zu\n", hits);
    CHECK(ok && hits == 1 && nl_copy_state_matches(p.context, &anchor) &&
          nl_semantic_snapshot(p.context, &after) &&
          !memcmp(&before, &after, sizeof(before)) &&
          nl_checked_copy_body_event_validate(t.owners[1], t.call_ids[1], 0,
                                              &t.events[1]) == NL_CHECK_OK &&
          emit(&p, NL_NODE_C_OK));
    nl_copy_state_destroy(&anchor);
    destroy(&p);
    return true;
}

static bool oom1481(const char *path, unsigned point)
{
    Program p = {0};
    CHECK(ready(path, &p));
    NLCopyCallAnchor anchor = {0};
    NLSemanticSnapshot before, after;
    CHECK(nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK &&
          nl_semantic_snapshot(p.context, &before));
    hits = text_allocs = join_hits = 0;
    if (point <= 2) {
        fault = point == 1 ? 1 : 0;
        join_fault = point == 2;
        NLCheckStatus s =
            nl_semantic_check_expression(p.context, p.call, &p.artifact, NULL);
        fault = 0;
        join_fault = false;
        printf("CHECK OOM point=%u hit=%zu join=%zu status=%d out=%p\n", point,
               hits, join_hits, (int)s, (void *)p.artifact);
        CHECK(hits == 1 && (point != 2 || join_hits == 1) &&
              s == NL_CHECK_OUT_OF_MEMORY && !p.artifact &&
              nl_copy_state_matches(p.context, &anchor) &&
              nl_semantic_snapshot(p.context, &after) &&
              !memcmp(&before, &after, sizeof(before)) && actual(&p));
    } else {
        CHECK(actual(&p));
        nl_copy_state_destroy(&anchor);
        CHECK(nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK &&
              nl_semantic_snapshot(p.context, &before));
        fault = point;
        bool ok = emit(&p, NL_NODE_C_OUT_OF_MEMORY);
        fault = 0;
        printf("emitter text ordinal=%u hits=%zu\n", point - 2, hits);
        CHECK(ok && hits == 1 && nl_copy_state_matches(p.context, &anchor) &&
              nl_semantic_snapshot(p.context, &after) &&
              !memcmp(&before, &after, sizeof(before)));
    }
    CHECK(emit(&p, NL_NODE_C_OK));
    nl_copy_state_destroy(&anchor);
    destroy(&p);
    return true;
}

static bool suite1481(const char *dir, const char *old, bool original_oom)
{
    char paths[6][1024];
    const char *names[] = {"P1468-B2-after-cleanup-in-success-arm.nl",
                           "Z-success-zero.nl",
                           "I-field-index-one.nl",
                           "N-wrong-domain.nl",
                           "N-missing-cleanup.nl",
                           "N-loan-escape.nl"};
    for (size_t i = 0; i < 6; ++i)
        CHECK(path1481(paths[i], sizeof paths[i], dir, names[i]));
    for (size_t i = 0; i < 3; ++i)
        CHECK(owned1481(paths[i], i == 2 ? 1 : 0, i == 1 ? 0 : 181));
    for (size_t i = 3; i < 6; ++i)
        CHECK(rejected1481(paths[i], paths[0]));
    CHECK(tamper1481(paths[0]) && suite(old));
    if (original_oom)
        for (unsigned point = 1; point <= 4; ++point)
            CHECK(oom1481(paths[0], point));
    CHECK(replay_oom1481(paths[0]));
    puts(
        "P1481 actual owned/current/negative/tamper/rollback + old A/B/C PASS");
    return true;
}
int main(int argc, char **argv)
{
    if (argc == 4 && !strcmp(argv[1], "suite"))
        return suite1481(argv[2], argv[3], true) ? 0 : 1;
    if (argc == 4 && !strcmp(argv[1], "focused"))
        return suite1481(argv[2], argv[3], false) ? 0 : 1;
    if (argc == 5 && !strcmp(argv[1], "owned"))
        return owned1481(argv[2], strtoul(argv[3], NULL, 10),
                         (unsigned)strtoul(argv[4], NULL, 10))
                   ? 0
                   : 1;
    return 2;
}
