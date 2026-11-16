#define main p1463_retained_main
#include "../p1463/probe.c"
#undef main

typedef struct {
    size_t joins, calls, loans;
    NLPlaceId root, child;
    NLIncarnationId incarnation;
    const NLCheckedFragment *owners[2];
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
            t->owners[t->calls++] = f;
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
int main(int argc, char **argv)
{
    if (argc == 5 && !strcmp(argv[1], "owned"))
        return owned1481(argv[2], strtoul(argv[3], NULL, 10),
                         (unsigned)strtoul(argv[4], NULL, 10))
                   ? 0
                   : 1;
    return 2;
}
